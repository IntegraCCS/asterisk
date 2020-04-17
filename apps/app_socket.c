/*
 * Asterisk -- A telephony toolkit for Linux.
 *
 * Socket Functions
 *
 * Sebastian Gutierrez scgm11@gmail.com
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License
 *
 *
 *
 *
 */

//
/*** MODULEINFO
    <defaultenabled>yes</defaultenabled>
    <support_level>core</support_level>
 ***/

#include <asterisk.h>

#include <asterisk/module.h>
#include <asterisk/app.h>
#include <asterisk/pbx.h>
#include <asterisk/channel.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>

#include <stdint.h>
#include <stdlib.h>


#define MAXBUF          4096

#define TABLELEN        63
#define BUFFFERLEN      128

#define ENCODERLEN      4
#define ENCODEROPLEN    0
#define ENCODERBLOCKLEN 3

#define PADDINGCHAR     '='
#define BASE64CHARSET   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*** DOCUMENTATION
    <application name="Socket" language="en_US">
        <synopsis>
            Connect to a Socket Server and Send and Receive Data.
            The Message is Encoded in Base64
            Each Message is Finished with \r\n\r\n
            For each Pair Var=Value\r\n creates Dialplan Variables
        </synopsis>
        <syntax>
        Example:
        exten=> 1234,1,Set(message=MessageID=1234\r\nMessageType=CallerIDJob\r\nPassword=0ADE123ED1C1C0634943504AF90AE6B9E9A799F7AA98B86808A486C5D91463EE\r\nPhoneNumber=8051290\r\nAddResponseID=true\r\n)
        exten=> 1234,2,Socket(190.146.106.225,60013,${message})
        exten=> 1234,3,NoOp(${SOCKETRES})
        </syntax>
        <description>
        <para>Retrieves data from the socket query.</para>
        </description>
    </application>
 ***/

/* Function prototypes */
int Base64Encode(char *input, char *output, int oplen);
int encodeblock(char *input, char *output, int oplen);
int Base64Decode(char *input, char *output, int oplen);
int decodeblock(char *input, char *output, int oplen);

static char *app = "Socket";



static int execute(struct ast_channel *chan, const char *data)
{

    int sockfd;
    struct sockaddr_in dest;
    char buffer[MAXBUF];
    struct hostent *serverhost;
    struct ast_hostent ahp;

    struct timeval timeout;      
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;


    char *parse;
        AST_DECLARE_APP_ARGS(args,
            AST_APP_ARG(Server);
            AST_APP_ARG(Port);
            AST_APP_ARG(Message);
        );


    //Check for arguments
    if (ast_strlen_zero(data)) {
        ast_log(LOG_WARNING, "Socket requires arguments (Server,Port,Message)\n");
        return -1;
    }

    parse = ast_strdupa(data);

    AST_STANDARD_APP_ARGS(args, parse);

    //Check for all needed arguments
    if (ast_strlen_zero(args.Server) || ast_strlen_zero(args.Port) || ast_strlen_zero(args.Message)) {
        ast_log(LOG_WARNING, "Missing argument to Socket (Server,Port,Message)\n");
        return -1;
    }

    //Open Socket for streaming
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
    {
        ast_log(LOG_ERROR, "Can´t Open Socket\n");
        return -1;
    }

    if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
        ast_log(LOG_ERROR, "ERROR SET RECEIVE TIMEOUT\n");

    if (setsockopt (sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
           ast_log(LOG_ERROR, "ERROR SET SEND TIMEOUT\n");


    memset(&dest,'\0',sizeof(dest));

    if ((dest.sin_addr.s_addr = inet_addr(args.Server)) == -1) {
        /* its a name rather than an ipnum */
        serverhost = ast_gethostbyname(args.Server, &ahp);
   

        if (serverhost == NULL) {
                ast_log(LOG_WARNING, "gethostbyname failed\n");
                return -1;
        }

        memmove(&dest.sin_addr, serverhost->h_addr, serverhost->h_length);
    
     }

   // Initialize server address/port struct
    dest.sin_family = AF_INET;
    dest.sin_port = htons(atoi(args.Port));


    //Connect to server
    if ( connect(sockfd, (struct sockaddr*)&dest, sizeof(dest)) != 0 )
    {
        ast_log(LOG_ERROR, "Can´t Connect to the Server\n");
        return 0;
    }

    //Change \r\n for respective UTF8
    ast_unescape_c(args.Message);

    ast_verb( 99, "app_socket: Message:  %s\n", args.Message);

    //Convert Message to Base64
    char messageb64[MAXBUF];
    memset(messageb64,'\0',MAXBUF);
    Base64Encode(args.Message,messageb64,sizeof(messageb64));

    ast_verb( 99, "app_socket: Message B64:  %s\n", messageb64);

    //Add protocol ending to message
    char aux[MAXBUF];
    memset(aux,'\0',MAXBUF);
    strcat(aux,messageb64);
    strcat(aux,"\r\n\r\n");

    //Send message through the Socket
    write(sockfd,aux,strlen(aux));

    //Get Response
    memset(buffer,'\0',MAXBUF);
    recv(sockfd, buffer, sizeof(buffer), 0);

    ast_verb( 99, "app_socket: Response B64:  %s\n", buffer);

    //Decode Response
    char aux2[MAXBUF];
    ast_copy_string(aux2, buffer, MAXBUF);
    memset(aux2,'\0',MAXBUF);
    Base64Decode(buffer, aux2, MAXBUF);

    ast_verb( 99, "app_socket: Response:  %s\n", aux2);

    //Close socket
    close(sockfd);

//For each Pair Var=Value\r\n creates Dialplan Variables
    char *delimiters= "\r\n=";
    char *running = NULL;
    char *variable = NULL;
    char *value = NULL;


    running = strdup (aux2);
    variable = strsep (&running, delimiters);
    while (variable != NULL) {
         value = strsep (&running, delimiters);
         strsep (&running, delimiters);
        if (variable != NULL && value != NULL && strcmp(variable,"") != 0 && strcmp(variable," ")) {
         pbx_builtin_setvar_helper(chan, variable, value);
         ast_verb( 9,"Variable: %s Value: %s\n", variable, value);
        }
         variable = strsep (&running, delimiters);
    }

    free(running);

    return 0;

}



int decodeblock(char *input, char *output, int oplen){
    int rc = 0;
    char decodedstr[ENCODERLEN + 1] = "";

    decodedstr[0] = input[0] << 2 | input[1] >> 4;
    decodedstr[1] = input[1] << 4 | input[2] >> 2;
    decodedstr[2] = input[2] << 6 | input[3] >> 0;
    strncat(output, decodedstr, oplen-strlen(output));

    return rc;
}

int Base64Decode(char *input, char *output, int oplen){
    char *charval = 0;
    char decoderinput[ENCODERLEN + 1] = "";
    char encodingtabe[TABLELEN + 1] = BASE64CHARSET;
    int index = 0, asciival = 0, computeval = 0, iplen = 0, rc = 0;

    iplen = strlen(input);
    while(index < iplen){
        asciival = (int)input[index];
        if(asciival == PADDINGCHAR){
            rc = decodeblock(decoderinput, output, oplen);
            break;
        }else{
            charval = strchr(encodingtabe, asciival);
            if(charval){
                decoderinput[computeval] = charval - encodingtabe;
                computeval = (computeval + 1) % 4;
                if(computeval == 0){
                    rc = decodeblock(decoderinput, output, oplen);
                    decoderinput[0] = decoderinput[1] =
                    decoderinput[2] = decoderinput[3] = 0;
                }
            }
        }
        index++;
    }

    return rc;
}

int encodeblock(char *input, char *output, int oplen){
    int rc = 0, iplen = 0;
    char encodedstr[ENCODERLEN + 1] = "";
    char encodingtabe[TABLELEN + 1] = BASE64CHARSET;

    iplen = strlen(input);
    encodedstr[0] = encodingtabe[ input[0] >> 2 ];
    encodedstr[1] = encodingtabe[ ((input[0] & 0x03) << 4) |
                                 ((input[1] & 0xf0) >> 4) ];
    encodedstr[2] = (iplen > 1 ? encodingtabe[ ((input[1] & 0x0f) << 2) |
                                              ((input[2] & 0xc0) >> 6) ] : PADDINGCHAR);
    encodedstr[3] = (iplen > 2 ? encodingtabe[ input[2] & 0x3f ] : PADDINGCHAR);
    strncat(output, encodedstr, oplen-strlen(output));

    return rc;
}

int Base64Encode(char *input, char *output, int oplen){
    int rc = 0;
    int index = 0, ipindex = 0, iplen = 0;
    char encoderinput[ENCODERBLOCKLEN + 1] = "";

    iplen = strlen(input);
    while(ipindex < iplen){
        for(index = 0; index < 3; index++){
            if(ipindex < iplen){
                encoderinput[index] = input[ipindex];
            }else{
                encoderinput[index] = 0;
            }
            ipindex++;
        }
        rc = encodeblock(encoderinput, output, oplen);
    }

    return rc;
}


static int unload_module (void)
{

    return ast_unregister_application (app);

}


static int load_module (void)
{

    return ast_register_application_xml(app, execute) ?
        AST_MODULE_LOAD_DECLINE : AST_MODULE_LOAD_SUCCESS;
}

AST_MODULE_INFO_STANDARD(ASTERISK_GPL_KEY, "Socket");
