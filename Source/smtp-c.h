/*
*********************************************************************************************************
*                                              uC/SMTPc
*                               Simple Mail Transfer Protocol (client)
*
*                    Copyright 2004-2020 Silicon Laboratories Inc. www.silabs.com
*
*                                 SPDX-License-Identifier: APACHE-2.0
*
*               This software is subject to an open source license and is distributed by
*                Silicon Laboratories Inc. pursuant to the terms of the Apache License,
*                    Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
*
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                             SMTP CLIENT
*
* Filename : smtp-c.h
* Version  : V2.01.00
*********************************************************************************************************
* Note(s)  : (1) This code implements a subset of the SMTP protocol (RFC 2821).  More precisely, the
*                following commands have been implemented:
*
*                  HELO
*                  AUTH (if enabled)
*                  MAIL
*                  RCPT
*                  DATA
*                  RSET
*                  NOOP
*                  QUIT
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                               MODULE
*
* Note(s) : (1) This header file is protected from multiple preprocessor inclusion through use of the
*               SMTPc present preprocessor macro definition.
*********************************************************************************************************
*/

#ifndef  SMTPc_PRESENT                                          /* See Note #1.                                         */
#define  SMTPc_PRESENT


/*
*********************************************************************************************************
*                                        SMTPc VERSION NUMBER
*
* Note(s) : (1) (a) The SMTPc module software version is denoted as follows :
*
*                       Vx.yy.zz
*
*                           where
*                                   V               denotes 'Version' label
*                                   x               denotes     major software version revision number
*                                   yy              denotes     minor software version revision number
*                                   zz              denotes sub-minor software version revision number
*
*               (b) The SMTPc software version label #define is formatted as follows :
*
*                       ver = x.yyzz * 100 * 100
*
*                           where
*                                   ver             denotes software version number scaled as an integer value
*                                   x.yyzz          denotes software version number, where the unscaled integer
*                                                       portion denotes the major version number & the unscaled
*                                                       fractional portion denotes the (concatenated) minor
*                                                       version numbers
*********************************************************************************************************
*/

#define  SMTPc_VERSION                                 20100u   /* See Note #1.                                         */


/*
*********************************************************************************************************
*                                               EXTERNS
*********************************************************************************************************
*/

#ifdef   SMTPc_MODULE
#define  SMTPc_EXT
#else
#define  SMTPc_EXT  extern
#endif


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*
* Note(s) : (1) The SMTPc module files are located in the following directories :
*
*               (a) \<Your Product Application>\smtp-c_cfg.h
*
*               (b) (1) \<Network Protocol Suite>\Source\net_*.*
*
*                   (2) If network security manager is to be used:
*
*                       (A) \<Network Protocol Suite>\Secure\net_secure_mgr.*
*
*                       (B) \<Network Protocol Suite>\Secure\<Network Security Suite>\net_secure.*
*
*               (c) \<SMTPc>\Source\smtp-c.h
*                                  \smtp-c.c
*
*                       where
*                               <Your Product Application>      directory path for Your Product's Application
*                               <Network Protocol Suite>        directory path for network protocol suite
*                               <SMTPc>                         directory path for SMTPc module
*
*           (2) CPU-configuration software files are located in the following directories :
*
*               (a) \<CPU-Compiler Directory>\cpu_*.*
*               (b) \<CPU-Compiler Directory>\<cpu>\<compiler>\cpu*.*
*
*                       where
*                               <CPU-Compiler Directory>        directory path for common CPU-compiler software
*                               <cpu>                           directory name for specific processor (CPU)
*                               <compiler>                      directory name for specific compiler
*
*           (3) NO compiler-supplied standard library functions SHOULD be used.
*
*               (a) Standard library functions are implemented in the custom library module(s) :
*
*                       \<Custom Library Directory>\lib_*.*
*
*                           where
*                                   <Custom Library Directory>      directory path for custom library software
*
*               (b) #### The reference to standard library header files SHOULD be removed once all custom
*                   library functions are implemented WITHOUT reference to ANY standard library function(s).
*
*           (4) Compiler MUST be configured to include as additional include path directories :
*
*               (a) '\<Your Product Application>\' directory                            See Note #1a
*
*               (b) (1) '\<Network Protocol Suite>\                                         See Note #1b1
*
*                   (2) (A) '\<Network Protocol Suite>\Secure\'                             See Note #1b2A
*                       (B) '\<Network Protocol Suite>\Secure\<Network Security Suite>\'    See Note #1b2B
*
*               (c) '\<SMTPc>\' directory                                               See Note #1c
*
*               (d) (1) '\<CPU-Compiler Directory>\'                  directory         See Note #2a
*                   (2) '\<CPU-Compiler Directory>\<cpu>\<compiler>\' directory         See Note #2b
*
*               (e) '\<Custom Library Directory>\' directory                            See Note #3a
*********************************************************************************************************
*/

#include  <cpu.h>                                               /* CPU Configuration              (see Note #2b)        */
#include  <cpu_core.h>                                          /* CPU Core Library               (see Note #2a)        */

#include  <lib_def.h>                                           /* Standard        Defines        (see Note #3a)        */
#include  <lib_str.h>                                           /* Standard String Library        (see Note #3a)        */


#include  <Source/net.h>                                        /* Network Protocol Suite         (see Note #1b)        */
#include  <smtp-c_cfg.h>
#include  <Source/net_app.h>
#include  <Source/net_sock.h>
#include  <Source/net_conn.h>
#include  <Source/net_ascii.h>
#include  <Source/net_util.h>
#include  <Modules/Common/net_base64.h>


#if 1                                                           /* See Note #3b.                                        */
#include  <stdio.h>
#endif


/*
*********************************************************************************************************
*                                                DEFINES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                           ERROR CODES DEFINES
*********************************************************************************************************
*/     
typedef enum smtpc_err {
    SMTPc_ERR_NONE                                 = 51000u,
    SMTPc_ERR_SOCK_OPEN_FAILED                     = 51001u,
    SMTPc_ERR_SOCK_CONN_FAILED                     = 51002u,
    SMTPc_ERR_NO_SMTP_SERV                         = 51003u,
    SMTPc_ERR_RX_FAILED                            = 51004u,
    SMTPc_ERR_TX_FAILED                            = 51005u,
    SMTPc_ERR_NULL_ARG                             = 51006u,
    SMTPc_ERR_LINE_TOO_LONG                        = 51007u,
    SMTPc_ERR_STR_TOO_LONG                         = 51008u,
    SMTPc_ERR_REP                                  = 51009u,
    SMTPc_ERR_BUF_TOO_SMALL                        = 51010u,
    SMTPc_ERR_ENCODE                               = 51011u,
    SMTPc_ERR_AUTH_FAILED                          = 51012u,
    SMTPc_ERR_SECURE_NOT_AVAIL                     = 51013u,

    SMTPc_ERR_REP_POS                              = 51000u,
    SMTPc_ERR_REP_INTER                            = 51013u,
    SMTPc_ERR_REP_NEG                              = 51014u,
    SMTPc_ERR_REP_TOO_SHORT                        = 51015u,
  
    SMTPc_ERR_INVALID_ADDR                         = 51016u,

} SMTPc_ERR;


/*
*********************************************************************************************************
*                                          SMTPc STRUCT DEFINES
*
* Note(s) : (1) Line length limit, as defined in RFC #2822.
*
*           (2) SMTPc_CFG_COMM_BUF_LEN is the length of the buffer used to receive replies from the SMTP
*               server.  As stated in RFC #2821, Section 'The SMTP Specifications, Additional Implementation
*               Issues, Sizes and Timeouts, Size limits and minimums', "The maximum total length of a
*               reply code and the <CRLF> is 512 characters".
*
*               This buffer is also used to build outgoing messages and MUST NOT be smaller than 1000
*               (see Note #1).
*
*           (3) Maximum length of key-value strings in structure SMTPc_KEY_VAL.
*
*           (4) As mentioned in RFC #2821, Section 'The SMTP Specifications, Additional
*               Implementation Issues, Sizes and Timeouts, Size limits and minimums', "The maximum
*               total length of a user name or other local-part is 64 characters [and] the maximum
*               total length of a domain name or number is 255 characters.".  Adding 2 for
*               '@' (see SMTPc_MBOX structure) and '\0'.
*
*           (5) Maximum length of content-type.
*
*           (6) From RFC #2822, Section 'Syntax, Fields definitions, Identification fields', "The message
*               identifier (msg-id) [field] is similar in syntax to an angle-addr construct".
*
*           (7) Size of ParamArray in structure SMTPc_MIME_ENTITY_HDR.
*
*           (8) Maximum length of attachment's name and description.
*********************************************************************************************************
*/

#define  SMTPc_LINE_LEN_LIM                             1000    /* See Note #1.                                         */

#define  SMTPc_COMM_BUF_LEN                             1024    /* See Note #2.                                         */

                                                                /* See Note #3.                                         */
#define  SMTPc_KEY_VAL_KEY_LEN                            30
#define  SMTPc_KEY_VAL_VAL_LEN                            30

                                                                /* See Note #4.                                         */
#define  SMTPc_MBOX_DOMAIN_NAME_LEN                      255
#define  SMTPc_MBOX_LOCAL_PART_LEN                        64
#define  SMTPc_MBOX_ADDR_LEN                   (SMTPc_MBOX_DOMAIN_NAME_LEN + SMTPc_MBOX_LOCAL_PART_LEN + 2)

                                                                /* See Note #5.                                         */
#define  SMTPc_MIME_CONTENT_TYPE_LEN                      20

                                                                /* See Note #6.                                         */
#define  SMTPc_MIME_ID_LEN                      SMTPc_MBOX_ADDR_LEN

                                                                /* See Note #7.                                         */
#define  SMTPc_MIME_MAX_KEYVAL                             1

                                                                /* See Note #8.                                         */
#define  SMTPc_ATTACH_NAME_LEN                            50
#define  SMTPc_ATTACH_DESC_LEN                            50

                                                                /* See Note #6.                                         */
#define  SMTPc_MSG_MSGID_LEN                    SMTPc_MBOX_ADDR_LEN



/*
*********************************************************************************************************
*                                         STATUS CODES DEFINES
*
* Note(s): (1) Reply codes are defined here and classified by category.  Note that this list is not meant
*              to be exhaustive.
*********************************************************************************************************
*/

                                                                /* ------------ POSITIVE PRELIMINARY REPLY ------------ */
#define  SMTPc_REP_POS_PRELIM_GRP                          1

                                                                /* ------------ POSITIVE COMPLETION REPLY ------------- */
#define  SMTPc_REP_POS_COMPLET_GRP                         2
#define  SMTPc_REP_220                                   220    /* Service ready.                                       */
#define  SMTPc_REP_221                                   221    /* Service closing transmission channel.                */
#define  SMTPc_REP_235                                   235    /* Authentication successful.                           */
#define  SMTPc_REP_250                                   250    /* Requested mail action okay, completed.               */
#define  SMTPc_REP_251                                   251    /* User not local; will forward to <forward-path>.      */

                                                                /* ------------ POSITIVE INTERMEDIATE REPLY ----------- */
#define  SMTPc_REP_POS_INTER_GRP                           3
#define  SMTPc_REP_354                                   354    /* Start mail input; end with <CRLF>.<CRLF>.            */

                                                                /* -------- TRANSIENT NEGATIVE COMPLETION REPLY ------- */
#define  SMTPc_REP_NEG_TRANS_COMPLET_GRP                   4
#define  SMTPc_REP_421                                   421    /* Service closing transmission channel.                */

                                                                /* -------- PERMANENT NEGATIVE COMPLETION REPLY ------- */
#define  SMTPc_REP_NEG_COMPLET_GRP                         5
#define  SMTPc_REP_503                                   503    /* Bad sequence of commands.                            */
#define  SMTPc_REP_504                                   504    /* Command parameter not implemented.                   */
#define  SMTPc_REP_535                                   535    /* Authentication failure.                              */
#define  SMTPc_REP_550                                   550    /* Requested action not taken: mailbox unavailable.     */
#define  SMTPc_REP_554                                   554    /* Transaction failed.                                  */


/*
*********************************************************************************************************
*                                      COMMANDS AND STRINGS DEFINES
*********************************************************************************************************
*/

#define  SMTPc_CMD_HELO                         "HELO"
#define  SMTPc_CMD_MAIL                         "MAIL"
#define  SMTPc_CMD_RCPT                         "RCPT"
#define  SMTPc_CMD_DATA                         "DATA"
#define  SMTPc_CMD_RSET                         "RSET"
#define  SMTPc_CMD_NOOP                         "NOOP"
#define  SMTPc_CMD_QUIT                         "QUIT"

#define  SMTPc_CMD_AUTH                         "AUTH"
#define  SMTPc_CMD_AUTH_MECHANISM_PLAIN         "PLAIN"


#define  SMTPc_CRLF                             "\x0D\x0A"
#define  SMTPc_EOM                              "\x0D\x0A.\x0D\x0A"
#define  SMTPc_CRLF_SIZE                         2u

#define  SMTPc_HDR_FROM                         "From: "
#define  SMTPc_HDR_SENDER                       "Sender: "
#define  SMTPc_HDR_TO                           "To: "
#define  SMTPc_HDR_REPLYTO                      "Reply-to: "
#define  SMTPc_HDR_CC                           "Cc: "
#define  SMTPc_HDR_SUBJECT                      "Subject: "

#define  SMTPc_TAG_IPv6                         "IPv6:"


/*
*********************************************************************************************************
*                                       BASE 64 ENCODER DEFINES
*
* Note(s) : (1) The maximum input buffer passed to the base 64 encoder depends of the configured maximum
*               lengths for the username and password.  Two additional characters are added to these
*               values to account for the delimiter (see 'smtp-c.c SMTPc_AUTH()  Note #2').
*
*           (2) The size of the output buffer the base 64 encoder produces is typically bigger than the
*               input buffer by a factor of (4 x 3).  However, when padding is necessary, up to 3
*               additional characters could by appended.  Finally, one more character is used to NULL
*               terminate the buffer.
*********************************************************************************************************
*/

#define  SMTPc_ENCODER_BASE64_DELIMITER_CHAR               0
                                                                /* See Note #1.                                         */
#define  SMTPc_ENCODER_BASE54_IN_MAX_LEN            (SMTPc_CFG_MBOX_NAME_DISP_LEN + SMTPc_CFG_MSG_SUBJECT_LEN + 2)

                                                                /* See Note #2.                                         */
#define  SMTPc_ENCODER_BASE64_OUT_MAX_LEN        ((((SMTPc_CFG_MBOX_NAME_DISP_LEN + SMTPc_CFG_MSG_SUBJECT_LEN + 2) * 4) / 3) + 4)


/*
*********************************************************************************************************
*                                             DATA TYPES
*
* Note(s): (1) From RFC #2821 'The SMTP Model, Terminology, Mail Objects', "SMTP transports a mail
*              object.  A mail object contains an envelope and content.  The SMTP content is sent
*              in the SMTP DATA protocol unit and has two parts: the headers and the body":
*
*                       |----------------------|
*                       |                      |
*                       |                      |  Envelope
*                       |                      |
*                       |======================|
*                       |                      |
*                       |  Headers             |
*                       |                      |
*                       |                      |
*                       |                      |
*                       |----------------------|  Content
*                       |                      |
*                       |  Body                |
*                       |                      |
*                       |                      |
*                       |                      |
*                       |----------------------|
*
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                         KEY-VALUE STRUCT DATA TYPES
*
* Note(s): (1) This structure makes room for additional MIME header fields (see RFC #2045, Section
*              'Additional MIME Header Fields').
*********************************************************************************************************
*/

typedef struct SMTPc_key_val
{
    CPU_CHAR  Key[SMTPc_KEY_VAL_KEY_LEN];                       /* Key (hdr field name).                                */
    CPU_CHAR  Val[SMTPc_KEY_VAL_VAL_LEN];                       /* Val associated with the preceding key.               */
} SMTPc_KEY_VAL;


/*
*********************************************************************************************************
*                               SMTP MAILBOX AND MAILBOX LIST DATA TYPES
*
* Note(s): (1) Structure representing an email address, as well as the name of its owner.
*********************************************************************************************************
*/

typedef struct SMTPc_mbox
{
    CPU_CHAR  NameDisp[SMTPc_CFG_MBOX_NAME_DISP_LEN];           /* Disp'd name of addr's owner.                         */
    CPU_CHAR  Addr    [SMTPc_MBOX_ADDR_LEN];                    /* Addr (local part '@' domain).                        */
} SMTPc_MBOX;


/*
*********************************************************************************************************
*                                        SMTP MIME ENTITY HEADER
*
* Note(s): (1) See RFC #2045 for details.
*
*          (2) Structure subject to change.  For instance, other data structures could be used to represent
*              "Encoding", etc.  The encoding could also be left to do by the application
*********************************************************************************************************
*/

typedef struct SMTPc_mime_entity_hdr
{
    CPU_CHAR       *ContentType[SMTPc_MIME_CONTENT_TYPE_LEN];   /* Description of contained body data (IANA assigned).  */
    SMTPc_KEY_VAL  *ParamArray[SMTPc_MIME_MAX_KEYVAL];          /* Additional param for specified content-type.         */
    void           *ContentEncoding;                            /* Content transfer encoding.                           */
    void          (*EncodingFnctPtr)(CPU_CHAR *,                /* Ptr to fnct performing the encoding of the           */
                                     NET_ERR  *);               /* attachment.                                          */
    CPU_CHAR        ID[SMTPc_MIME_ID_LEN];                      /* Unique attachment id.                                */
} SMTPc_MIME_ENTITY_HDR;


/*
*********************************************************************************************************
*                        SMTP MESSAGE ATTACHMENT AND ATTACHMENT LIST DATA TYPES
*
* Note(s): (1) Attachments are not going to be supported in the first version of the uCSMTPc module.
*              Hence, the format of this structure will surely change to accommodate for possible
*              file system access.
*********************************************************************************************************
*/

typedef struct SMTPc_attach
{
    SMTPc_MIME_ENTITY_HDR   MIMEPartHdrStruct;                  /* MIME content hdr for this attachment.                */
    CPU_CHAR                Name[SMTPc_ATTACH_NAME_LEN];        /* Name of attachment inserted in the message.          */
    CPU_CHAR                Desc[SMTPc_ATTACH_DESC_LEN];        /* Optional attachment description.                     */
    void                   *AttachData;                         /* Ptr to beginning of body (data of the entity).       */
    CPU_INT32U              Size ;                              /* Size of data in octets.                              */

} SMTPc_ATTACH;


/*
*********************************************************************************************************
*                                          SMTP MSG Structure
*
* Note(s): (1) A mail object is represented in this module by the structure SMTPc_MSG.  This
*              structure contains all the necessary information to generate the mail object
*              and to send it to the SMTP server.  More specifically, it encapsulates the various
*              addresses of the sender and recipients, MIME information, the message itself, and
*              finally the eventual attachments.
*********************************************************************************************************
*/

typedef struct SMTPc_msg
{
    SMTPc_MBOX             *From;                               /* "From" field     (1:1).                              */
    SMTPc_MBOX             *ToArray[SMTPc_CFG_MSG_MAX_TO];      /* "To" field       (1:*).                              */
    SMTPc_MBOX             *ReplyTo;                            /* "Reply-to" field (0:1).                              */
    SMTPc_MBOX             *Sender;                             /* "Sender" field   (0:1).                              */
    SMTPc_MBOX             *CCArray[SMTPc_CFG_MSG_MAX_CC];      /* "CC" field       (0:*).                              */
    SMTPc_MBOX             *BCCArray[SMTPc_CFG_MSG_MAX_BCC];    /* "BCC" field      (0:*).                              */
    CPU_CHAR                MsgID[SMTPc_MSG_MSGID_LEN];         /* Unique msg id.                                       */
    SMTPc_MIME_ENTITY_HDR   MIMEMsgHdrStruct;                   /* Mail obj MIME content headers.                       */
    CPU_CHAR               *Subject;                            /* Subject of msg.                                      */
                                                                /* List of attachment(s), if any.                       */
    SMTPc_ATTACH           *AttachArray[SMTPc_CFG_MSG_MAX_ATTACH];
    CPU_CHAR               *ContentBodyMsg;                     /* Data of the mail obj content's body.                 */
    CPU_INT32U              ContentBodyMsgLen;                  /* Size (in octets) of buf pointed by ContentBodyMsg.   */
} SMTPc_MSG;


/*
*********************************************************************************************************
*                                          GLOBAL VARIABLES
*********************************************************************************************************
*/

SMTPc_EXT  CPU_CHAR  SMTPc_Comm_Buf[SMTPc_COMM_BUF_LEN];


/*
*********************************************************************************************************
*                                          FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void         SMTPc_SendMail    (CPU_CHAR                *p_host_name,
                                CPU_INT16U               port,
                                CPU_CHAR                *p_username,
                                CPU_CHAR                *p_pwd,
                                NET_APP_SOCK_SECURE_CFG *p_secure_cfg,
                                SMTPc_MSG               *p_msg,
                                SMTPc_ERR               *p_err);

                                                                /* ---------------- SMTP SESSION FNCTS ---------------- */
NET_SOCK_ID  SMTPc_Connect     (CPU_CHAR                *p_host_name,
                                CPU_INT16U               port,
                                CPU_CHAR                *p_username,
                                CPU_CHAR                *p_pwd,
                                NET_APP_SOCK_SECURE_CFG *p_secure_cfg,
                                SMTPc_ERR               *p_err);


void         SMTPc_SendMsg     (NET_SOCK_ID              sock_id,
                                SMTPc_MSG               *msg,
                                SMTPc_ERR               *perr);

void         SMTPc_Disconnect  (NET_SOCK_ID              sock_id,
                                SMTPc_ERR               *perr);


                                                                /* -------------------- UTIL FNCTS -------------------- */
void         SMTPc_SetMbox     (SMTPc_MBOX              *mbox,
                                CPU_CHAR                *name,
                                CPU_CHAR                *addr,
                                SMTPc_ERR               *perr);

void         SMTPc_SetMsg      (SMTPc_MSG               *msg,
                                SMTPc_ERR               *perr);


/*
*********************************************************************************************************
*                                              TRACING
*********************************************************************************************************
*/
                                                                /* Trace level, default to TRACE_LEVEL_OFF              */
#ifndef  TRACE_LEVEL_OFF
#define  TRACE_LEVEL_OFF                                 0
#endif

#ifndef  TRACE_LEVEL_INFO
#define  TRACE_LEVEL_INFO                                1
#endif

#ifndef  TRACE_LEVEL_DBG
#define  TRACE_LEVEL_DBG                                 2
#endif

#ifndef  SMTPc_TRACE_LEVEL
#define  SMTPc_TRACE_LEVEL                      TRACE_LEVEL_OFF
#endif

#ifndef  SMTPc_TRACE
#define  SMTPc_TRACE                            printf
#endif

#if    ((defined(SMTPc_TRACE))       && \
        (defined(SMTPc_TRACE_LEVEL)) && \
        (SMTPc_TRACE_LEVEL >= TRACE_LEVEL_INFO) )

    #if  (SMTPc_TRACE_LEVEL >= TRACE_LEVEL_DBG)
        #define  SMTPc_TRACE_DBG(msg)     SMTPc_TRACE  msg
    #else
        #define  SMTPc_TRACE_DBG(msg)
    #endif

    #define  SMTPc_TRACE_INFO(msg)        SMTPc_TRACE  msg

#else
    #define  SMTPc_TRACE_DBG(msg)
    #define  SMTPc_TRACE_INFO(msg)
#endif


/*
*********************************************************************************************************
*                                         CONFIGURATION ERRORS
*********************************************************************************************************
*/

#ifndef  SMTPc_CFG_IPPORT
#error  "SMTPc_CFG_IPPORT not #define'd in 'smtp-c_cfg.h' see template file in package named 'smtp-c_cfg.h'"
#endif


#ifndef  SMTPc_CFG_IPPORT_SECURE
#error  "SMTPc_CFG_IPPORT_SECURE not #define'd in 'smtp-c_cfg.h'see template file in package named 'smtp-c_cfg.h'"
#endif


#ifndef  SMTPc_CFG_MAX_CONN_REQ_TIMEOUT_MS
#error  "SMTPc_CFG_MAX_CONN_REQ_TIMEOUT_MS not #define'd in 'smtp-c_cfg.h' see template file in package named 'smtp-c_cfg.h'"
#endif


#ifndef  SMTPc_CFG_MAX_CONN_CLOSE_TIMEOUT_MS
#error  "SMTPc_CFG_MAX_CONN_CLSOE_TIMEOUT_MS not #define'd in 'smtp-c_cfg.h' see template file in package named 'smtp-c_cfg.h'"
#endif


#ifndef  SMTPc_CFG_AUTH_EN
#error  "SMTPc_CFG_AUTH_EN not #define'd in 'smtp-c_cfg.h [MUST be DEF_DISABLED || DEF_ENABLED]"
#elif  ((SMTPc_CFG_AUTH_EN != DEF_DISABLED) && \
        (SMTPc_CFG_AUTH_EN != DEF_ENABLED ))
#error  "SMTPc_CFG_AUTH_EN illegally #define'd in 'smtp-c_cfg.h' [MUST be DEF_DISABLED || DEF_ENABLED]"
#endif


#ifndef  SMTPc_CFG_USERNAME_MAX_LEN
#error  "SMTPc_CFG_USERNAME_MAX_LEN not #define'd in 'smtp-c_cfg.h' see template file in package named 'smtp-c_cfg.h'"
#elif  ((SMTMc_CFG_USERNAME_MAX_LEN <                   0) || \
        (SMTMc_CFG_USERNAME_MAX_LEN > DEF_INT_08U_MAX_VAL))
#error  "SMTMc_CFG_USERNAME_MAX_LEN illegally #define'd in 'smtp-c_cfg.h'[MUST be >= 0 && <= 255]"
#endif


#ifndef  SMTPc_CFG_PW_MAX_LEN
#error  "SMTPc_CFG_PW_MAX_LEN not #define'd in 'smtp-c_cfg.h' see template file in package named 'smtp-c_cfg.h'"
#elif  ((SMTPc_CFG_PW_MAX_LEN <                   0) || \
        (SMTPc_CFG_PW_MAX_LEN > DEF_INT_08U_MAX_VAL))
#error  "SMTPc_CFG_PW_MAX_LEN illegally #define'd in 'smtp-c_cfg.h' [MUST be >= 0 && <= 255]"
#endif



#ifndef  SMTPc_CFG_MBOX_NAME_DISP_LEN
#error  "SMTPc_CFG_MBOX_NAME_DISP_LEN not #define'd in 'smtp-c_cfg.h' see template file in package named 'smtp-c_cfg.h'"
#elif  ((SMTPc_CFG_MBOX_NAME_DISP_LEN <                   0) || \
        (SMTPc_CFG_MBOX_NAME_DISP_LEN > DEF_INT_08U_MAX_VAL))
#error  "SMTPc_CFG_MBOX_NAME_DISP_LEN illegally #define'd in 'smtp-c_cfg.h' [MUST be >= 0 && <= 255]"
#endif


#ifndef  SMTPc_CFG_MSG_SUBJECT_LEN
#error  "SMTPc_CFG_MSG_SUBJECT_LEN not #define'd in 'smtp-c_cfg.h' see template file in package named 'smtp-c_cfg.h'"
#elif  ((SMTPc_CFG_MSG_SUBJECT_LEN <                   0) || \
        (SMTPc_CFG_MSG_SUBJECT_LEN > DEF_INT_08U_MAX_VAL))
#error  "SMTPc_CFG_MSG_SUBJECT_LEN illegally #define'd in 'smtp-c_cfg.h' [MUST be >= 0 && <= 255]"
#endif


#ifndef  SMTPc_CFG_MSG_MAX_TO
#error  "SMTPc_CFG_MSG_MAX_TO not #define'd in 'smtp-c_cfg.h' see template file in package named 'smtp-c_cfg.h'"
#elif  ((SMTPc_CFG_MSG_MAX_TO <                   0) || \
        (SMTPc_CFG_MSG_MAX_TO > DEF_INT_08U_MAX_VAL))
#error  "SMTPc_CFG_MSG_MAX_TO illegally #define'd in 'smtp-c_cfg.h' [MUST be >= 0 && <= 255]"
#endif


#ifndef  SMTPc_CFG_MSG_MAX_CC
#error  "SMTPc_CFG_MSG_MAX_CC not #define'd in 'smtp-c_cfg.h' see template file in package named 'smtp-c_cfg.h'"
#elif  ((SMTPc_CFG_MSG_MAX_CC <                   0) || \
        (SMTPc_CFG_MSG_MAX_CC > DEF_INT_08U_MAX_VAL))
#error  "SMTPc_CFG_MSG_MAX_CC illegally #define'd in 'smtp-c_cfg.h' [MUST be >= 0 && <= 255]"
#endif


#ifndef  SMTPc_CFG_MSG_MAX_BCC
#error  "SMTPc_CFG_MSG_MAX_BCC not #define'd in 'smtp-c_cfg.h' see template file in package named 'smtp-c_cfg.h'"
#elif  ((SMTPc_CFG_MSG_MAX_BCC <                   0) || \
        (SMTPc_CFG_MSG_MAX_BCC > DEF_INT_08U_MAX_VAL))
#error  "SMTPc_CFG_MSG_MAX_BCC illegally #define'd in 'smtp-c_cfg.h' [MUST be >= 0 && <= 255]"
#endif


#ifndef  SMTPc_CFG_MSG_MAX_ATTACH
#error  "SMTPc_CFG_MSG_MAX_ATTACH not #define'd in 'smtp-c_cfg.h' see template file in package named 'smtp-c_cfg.h'"
#elif  ((SMTPc_CFG_MSG_MAX_ATTACH <                   0) || \
        (SMTPc_CFG_MSG_MAX_ATTACH > DEF_INT_08U_MAX_VAL))
#error  "SMTPc_CFG_MSG_MAX_ATTACH illegally #define'd in 'smtp-c_cfg.h' [MUST be >= 0 && <= 255]"
#endif


/*
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*/

#endif                                                          /* End of SMTPc module include.                         */

