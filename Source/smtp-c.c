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
* Filename : smtp-c.c
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
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define    MICRIUM_SOURCE
#define    SMTPc_MODULE
#include  "smtp-c.h"


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            LOCAL TABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          FUNCTION PROTOTYPES
*********************************************************************************************************
*/

                                                                /* --------------------- RX FNCT'S -------------------- */
static  CPU_CHAR    *SMTPc_RxReply      (NET_SOCK_ID   sock_id,
                                         SMTPc_ERR    *perr);

static  void         SMTPc_ParseReply   (CPU_CHAR     *server_reply,
                                         CPU_INT32U   *completion_code,
                                         SMTPc_ERR    *perr);

                                                                /* --------------------- TX FNCT'S -------------------- */
static  void         SMTPc_SendBody     (NET_SOCK_ID   sock_id,
                                         SMTPc_MSG    *msg,
                                         SMTPc_ERR    *perr);

static  void         SMTPc_QueryServer  (NET_SOCK_ID   sock_id,
                                         CPU_CHAR     *query,
                                         CPU_INT32U    len,
                                         SMTPc_ERR    *perr);

                                                                /* ------------------- UTIL FNCT'S ------------------- */
static  CPU_INT32U   SMTPc_BuildHdr     (NET_SOCK_ID   sock_id,
                                         CPU_CHAR     *buf,
                                         CPU_INT32U    buf_size,
                                         CPU_INT32U    buf_wr_ix,
                                         CPU_CHAR     *hdr,
                                         CPU_CHAR     *val,
                                         CPU_INT32U   *line_len,
                                         SMTPc_ERR    *perr);

                                                                /* -------------------- CMD FNCT'S ------------------- */
static  CPU_CHAR    *SMTPc_HELO         (NET_SOCK_ID   sock_id,
                                         CPU_INT32U   *completion_code,
                                         SMTPc_ERR    *perr);

#if (SMTPc_CFG_AUTH_EN == DEF_ENABLED)
static  CPU_CHAR    *SMTPc_AUTH         (NET_SOCK_ID   sock_id,
                                         CPU_CHAR     *username,
                                         CPU_CHAR     *pw,
                                         CPU_INT32U   *completion_code,
                                         SMTPc_ERR    *perr);
#endif

static  CPU_CHAR    *SMTPc_MAIL         (NET_SOCK_ID   sock_id,
                                         CPU_CHAR     *from,
                                         CPU_INT32U   *completion_code,
                                         SMTPc_ERR    *perr);

static  CPU_CHAR    *SMTPc_RCPT         (NET_SOCK_ID   sock_id,
                                         CPU_CHAR     *to,
                                         CPU_INT32U   *completion_code,
                                         SMTPc_ERR    *perr);

static  CPU_CHAR    *SMTPc_DATA         (NET_SOCK_ID   sock_id,
                                         CPU_INT32U   *completion_code,
                                         SMTPc_ERR    *perr);

static  CPU_CHAR    *SMTPc_RSET         (NET_SOCK_ID   sock_id,
                                         CPU_INT32U   *completion_code,
                                         SMTPc_ERR    *perr);

static  CPU_CHAR    *SMTPc_QUIT         (NET_SOCK_ID   sock_id,
                                         CPU_INT32U   *completion_code,
                                         SMTPc_ERR    *perr);


/*
*********************************************************************************************************
*                                            SMTPc_Connect()
*
* Description : (1) Establish a TCP connection to the SMTP server and initiate the SMTP session.
*
*                   (a) Determine port
*                   (b) Open the socket
*                   (c) Receive server's reply & validate
*                   (d) Initiate SMTP session
*                   (e) Authenticate client, if applicable
*
*
* Argument(s) : p_host_name     Pointer to host name of the SMTP server to contact. Can be also an IP address.
*
*               port            TCP port to use, or '0' if SMTPc_DFLT_PORT.
*
*               p_username      Pointer to user name, if authentication enabled.
*
*               p_pwd           Pointer to password,  if authentication enabled.
*
*               p_secure_cfg    Pointer to the secure configuration (TLS/SSL):
*
*                                       DEF_NULL, if no security enabled.
*                                       Pointer to a structure that contains the parameters.
*
*               p_err           Pointer to variable that will hold the return error code from this
*                               function :
*
*                               SMTPc_ERR_NONE                      No error, TCP connection established.
*                               SMTPc_ERR_NULL_ARG                  Argument 'username'/'pw' passed a NULL pointer.
*                               SMTPc_ERR_SOCK_OPEN_FAILED          Error opening socket.
*                               SMTPc_ERR_SOCK_CONN_FAILED          Error connecting to server.
*                               SMTPc_ERR_RX_FAILED                 Error receiving server reply.
*                               SMTPc_ERR_REP                       Error with reply.
*                               SMTPc_ERR_SECURE_NOT_AVAIL          No secure mode available.
*
*                                                                   -------- RETURNED BY SMTPc_AUTH : --------
*                               SMTPc_ERR_ENCODE                    Error encoding credentials.
*                               SMTPc_ERR_AUTH_FAILED               Error authenticating user.
*
*
* Return(s)   : Socket descriptor/handle identifier, if NO error.
*
*               -1,                                  otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : (2) Network security manager MUST be available & enabled to initialize the server in
*                   secure mode.
*
*               (3) If anything goes wrong while trying to connect to the server, the socket is
*                   closed by calling NetSock_Close.  Hence, all data structures are returned to
*                   their original state in case of a failure to establish the TCP connection.
*
*                   If the failure occurs when initiating the session, the application is responsible
*                   of the appropriate action(s) to be taken.
*
*               (4) If authentication is disabled (SMTPc_CFG_AUTH_EN set to DEF_ENABLED), the 'p_username'
*                   and 'p_pwd' parameters should be passed a NULL pointer.
*
*               (5) The server will send a 220 "Service ready" reply when the connection is completed.
*                   The SMTP protocol allows a server to formally reject a transaction while still
*                   allowing the initial connection by responding with a 554 "Transaction failed"
*                   reply.
*
*               (6) The Plain-text (PLAIN) authentication mechanism is implemented here.  However, it
*                   takes some liberties from RFC #4964, section 4 'The AUTH Command', stating the "A
*                   server implementation MUST implement a configuration in which it does not permit
*                   any plaintext password mechanisms, unless either the STARTTLS command has been
*                   negotiated or some other mechanism that protects the session from password snooping
*                   has been provided".
*
*                   Since this client does not support SSL or TLS, or any other protection against
*                   password snooping, it relies on the server NOT to fully follow RFC #4954 in order
*                   to be successful.
*********************************************************************************************************
*/

NET_SOCK_ID  SMTPc_Connect (CPU_CHAR                *p_host_name,
                            CPU_INT16U               port,
                            CPU_CHAR                *p_username,
                            CPU_CHAR                *p_pwd,
                            NET_APP_SOCK_SECURE_CFG *p_secure_cfg,
                            SMTPc_ERR               *p_err)
{
    NET_SOCK_ID     sock_id;
    CPU_INT32U      completion_code;
    CPU_CHAR       *reply;
    NET_ERR         err_net;
    NET_SOCK_ADDR   socket_addr;
    CPU_INT16U      port_server;

                                                                /* ------------------ VALIDATE PTR -------------------- */
#if (SMTPc_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
#if (SMTPc_CFG_AUTH_EN == DEF_ENABLED)
    if ((p_username == (CPU_CHAR *)0) ||
        (p_pwd      == (CPU_CHAR *)0)) {
       *p_err = SMTPc_ERR_NULL_ARG;
        return (NET_SOCK_ID_NONE);
    }
#else
   (void)&p_username;                                           /* Prevent 'variable unused' compiler warnings.         */
   (void)&p_pwd;
#endif

#ifndef  NET_SECURE_MODULE_EN                                   /* See Note #2.                                         */
    if (p_secure_cfg != DEF_NULL) {
       *p_err = SMTPc_ERR_SECURE_NOT_AVAIL;
        return NET_SOCK_ID_NONE;
    }
#endif

#endif
                                                                /* ------------------ DETERMINE PORT ------------------ */
    if (port != 0) {
        port_server = port;
    } else {
        if (p_secure_cfg != DEF_NULL) {                         /* Set the port according to the secure mode cfg.       */
            port_server = SMTPc_CFG_IPPORT_SECURE;
        } else {
            port_server = SMTPc_CFG_IPPORT;
        }
    }
                                                                /* ----------------- OPEN CLIENT STREAM --------------- */
    NetApp_ClientStreamOpenByHostname(&sock_id,
                                       p_host_name,
                                       port_server,
                                      &socket_addr,
                                       p_secure_cfg,
                                       SMTPc_CFG_MAX_CONN_REQ_TIMEOUT_MS,
                                      &err_net);
    if ( err_net != NET_APP_ERR_NONE) {
       *p_err = SMTPc_ERR_SOCK_CONN_FAILED;
        return NET_SOCK_ID_NONE;
    }
                                                                /* ---------------- CFG SOCK BLOCK OPT ---------------- */
    (void)NetSock_CfgBlock(sock_id, NET_SOCK_BLOCK_SEL_BLOCK, &err_net);
    if (err_net != NET_SOCK_ERR_NONE) {
        NetApp_SockClose(sock_id,
                         SMTPc_CFG_MAX_CONN_CLOSE_TIMEOUT_MS,
                         &err_net);
       *p_err = SMTPc_ERR_SOCK_CONN_FAILED;
        return NET_SOCK_ID_NONE;
    }



                                                                /* ---------- RX SERVER'S RESPONSE & VALIDATE --------- */
    reply = SMTPc_RxReply(sock_id, p_err);                      /* See Note #5.                                         */
    if (*p_err != SMTPc_ERR_NONE) {
        NetApp_SockClose(sock_id,
                         SMTPc_CFG_MAX_CONN_CLOSE_TIMEOUT_MS,
                         &err_net);
       *p_err = SMTPc_ERR_RX_FAILED;
        return NET_SOCK_ID_NONE;
    }

    SMTPc_ParseReply(reply, &completion_code, p_err);
    switch (*p_err) {
        case SMTPc_ERR_REP_POS:                                 /* If any pos rep...                                    */
            *p_err = SMTPc_ERR_NONE;                            /* ... no err.                                          */
             break;

        case SMTPc_ERR_REP_INTER:
        case SMTPc_ERR_REP_NEG:
        case SMTPc_ERR_REP_TOO_SHORT:
        default:
             NetApp_SockClose(sock_id,
                              SMTPc_CFG_MAX_CONN_CLOSE_TIMEOUT_MS,
                              &err_net);
            *p_err = SMTPc_ERR_REP;
             return NET_SOCK_ID_NONE;
    }
                                                                /* -------------- INITIATE SMTP SESSION --------------- */
    reply = SMTPc_HELO(sock_id, &completion_code, p_err);
    if (*p_err != SMTPc_ERR_NONE) {
        SMTPc_Disconnect(sock_id, p_err);
        return NET_SOCK_ID_NONE;;
    }

                                                                /* -------------------- AUTH CLIENT ------------------- */
                                                                /* See Note #6.                                         */
#if (SMTPc_CFG_AUTH_EN == DEF_ENABLED)
     SMTPc_AUTH((NET_SOCK_ID ) sock_id,
                (CPU_CHAR   *) p_username,
                (CPU_CHAR   *) p_pwd,
                (CPU_INT32U *)&completion_code,
                (SMTPc_ERR  *) p_err);
    if (*p_err != SMTPc_ERR_NONE) {
        SMTPc_Disconnect(sock_id, p_err);
        return NET_SOCK_ID_NONE;;
    }
#endif

    return (sock_id);
}


/*
*********************************************************************************************************
*                                            SMTPc_SendMsg()
*
* Description : (1) Send a message (an instance of the SMTPc_MSG structure) to the SMTP server.
*
*                   (a) Invoke the MAIL command
*                   (b) Invoke the RCPT command for every recipient
*                   (c) Invoke the DATA command
*                   (d) Build and send the actual data
*
*
* Argument(s) : sock_id         Socket ID.
*               p_msg           SMTPc_MSG structure encapsulating the message to send.
*               p_err           Pointer to variable that will hold the return error code from this
*                               function :
*
*                               SMTPc_ERR_NONE                      No error.
*                               SMTPc_ERR_NULL_ARG                  Argument 'p_msg' passed a NULL pointer.
*                               SMTPc_ERR_RX_FAILED                 Error receiving server reply.
*                               SMTPc_ERR_REP                       Error with reply.
*
*                                                                   ------- RETURNED BY SMTPc_MAIL() : -------
*                                                                   ------- RETURNED BY SMTPc_RCPT() : -------
*                                                                   ------- RETURNED BY SMTPc_DATA() : -------
*                               SMTPc_ERR_TX_FAILED                 Error querying server.
*                               SMTPc_ERR_RX_FAILED                 Error receiving server reply.
*                               SMTPc_ERR_REP                       Error with reply.
*
*                                                                   ----- RETURNED BY SMTPc_SendBody() : -----
*                               SMTPc_ERR_TX_FAILED                 Error querying server.
*                               SMTPc_ERR_LINE_TOO_LONG             Line limit exceeded.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : (2) The function SMTPc_SetMsg has to be called before being able to send a message.
*
*               (3) The message has to have at least one receiver, either "To", "CC", or "BCC".
*********************************************************************************************************
*/

void  SMTPc_SendMsg (NET_SOCK_ID   sock_id,
                     SMTPc_MSG    *p_msg,
                     SMTPc_ERR    *p_err)
{
    CPU_INT08U  i;
    CPU_INT32U  completion_code;


                                                                /* See Note #2.                                         */
    if (p_msg->From == (SMTPc_MBOX *)0) {
         SMTPc_TRACE_DBG(("Error SMTPc_SendMsg.  NULL from parameter\n\r"));
        *p_err = SMTPc_ERR_NULL_ARG;
         return;
    }

                                                                /* See Note #3.                                         */
    if ((p_msg->ToArray[0]  == (SMTPc_MBOX *)0) &&
        (p_msg->CCArray[0]  == (SMTPc_MBOX *)0) &&
        (p_msg->BCCArray[0] == (SMTPc_MBOX *)0)) {
         SMTPc_TRACE_DBG(("Error SMTPc_SendMsg.  NULL parameter(s)\n\r"));
        *p_err = SMTPc_ERR_NULL_ARG;
         return;
    }

                                                                /* --------------- INVOKE THE MAIL CMD ---------------- */
    SMTPc_MAIL(sock_id, p_msg->From->Addr, &completion_code, p_err);
    if (*p_err != SMTPc_ERR_NONE) {
         SMTPc_TRACE_DBG(("Error MAIL.  Code: %u\n\r", (unsigned int)completion_code));
         if ((completion_code != SMTPc_REP_421) &&
             (completion_code != SMTPc_REP_221)) {
             SMTPc_RSET(sock_id, &completion_code, p_err);
         }
         return;
    }

                                                                /* --------------- INVOKE THE RCTP CMD ---------------- */
                                                                /* The RCPT cmd is tx'd for every recipient,            */
                                                                /* including CCs & BCCs.                                */
    for (i = 0; i < SMTPc_CFG_MSG_MAX_TO; i++) {
        if (p_msg->ToArray[i] == (SMTPc_MBOX *)0) {
            break;
        }

        SMTPc_RCPT(sock_id, p_msg->ToArray[i]->Addr, &completion_code, p_err);
        if (*p_err != SMTPc_ERR_NONE) {
             SMTPc_TRACE_DBG(("Error RCPT (TO %u).  Code: %u\n\r", (unsigned int)i, (unsigned int)completion_code));
             SMTPc_RSET(sock_id, &completion_code, p_err);      /* RSET p_msg if invalid RCPT fails.                      */
             return;
        }
    }

                                                                /* CCs                                                  */
    for (i = 0; i < SMTPc_CFG_MSG_MAX_CC; i++) {
        if (p_msg->CCArray[i] == (SMTPc_MBOX *)0) {
            break;
        }
        SMTPc_RCPT(sock_id, p_msg->CCArray[i]->Addr, &completion_code, p_err);
        if (*p_err != SMTPc_ERR_NONE) {
             SMTPc_TRACE_DBG(("Error RCPT (CC %u).  Code: %u\n\r", (unsigned int)i, (unsigned int)completion_code));
             if ((completion_code != SMTPc_REP_421) &&
                 (completion_code != SMTPc_REP_221)) {
                 SMTPc_RSET(sock_id, &completion_code, p_err);  /* RSET p_msg if invalid RCPT fails.                      */
             }
             return;
        }
    }

                                                                /* BCCs                                                 */
    for (i = 0; i < SMTPc_CFG_MSG_MAX_BCC; i++) {
        if (p_msg->BCCArray[i] == (SMTPc_MBOX *)0) {
            break;
        }

        SMTPc_RCPT(sock_id, p_msg->BCCArray[i]->Addr, &completion_code, p_err);
        if (*p_err != SMTPc_ERR_NONE) {
             SMTPc_TRACE_DBG(("Error RCPT (BCC %u).  Code: %u\n\r", (unsigned int)i, (unsigned int)completion_code));
             if ((completion_code != SMTPc_REP_421) &&          /* RSET p_msg if invalid RCPT fails.                      */
                 (completion_code != SMTPc_REP_221)) {
                 SMTPc_RSET(sock_id, &completion_code, p_err);
             }
             return;
        }
    }

                                                                /* --------------- INVOKE THE DATA CMD ---------------- */
    SMTPc_DATA(sock_id, &completion_code, p_err);
    if (*p_err != SMTPc_ERR_NONE) {
         SMTPc_TRACE_DBG(("Error DATA.  Code: %u\n\r", (unsigned int)completion_code));
         if ((completion_code != SMTPc_REP_421) &&
             (completion_code != SMTPc_REP_221)) {
             SMTPc_RSET(sock_id, &completion_code, p_err);
         }
         return;
    }

                                                                /* ----------- BUILD & SEND THE ACTUAL MSG ------------ */
    SMTPc_SendBody(sock_id, p_msg, p_err);
    if (*p_err != SMTPc_ERR_NONE) {
         SMTPc_TRACE_DBG(("Error SMTPc_SendBody.  Error: %u\n\r", (unsigned int)*p_err));
    }
}


/*
*********************************************************************************************************
*                                          SMTPc_Disconnect()
*
* Description : (1) Close the connection between client and server.
*
*                   (a) Send QUIT command
*                   (b) Close socket
*
*
* Argument(s) : sock_id         Socket ID.
*               p_err            Pointer to variable that will hold the return error code from this
*                               function :
*
*                               SMTPc_ERR_NONE                      No error.
*
* Return(s)   : none.
*
* Caller(s)   : Application,
*               SMTPc_Connect().
*
* Note(s)     : (2) The receiver (client) MUST NOT intentionally close the transmission channel until
*                   it receives and replies to a QUIT command.
*
*               (3) The receiver of the QUIT command MUST send an OK reply, and then close the
*                   transmission channel.
*********************************************************************************************************
*/

void  SMTPc_Disconnect (NET_SOCK_ID   sock_id,
                        SMTPc_ERR    *p_err)
{
    CPU_INT32U  completion_code;
    NET_ERR     err;


    SMTPc_QUIT(sock_id, &completion_code, p_err);

    NetApp_SockClose(sock_id,
                     SMTPc_CFG_MAX_CONN_CLOSE_TIMEOUT_MS,
                    &err);

   *p_err = SMTPc_ERR_NONE;
}


/*
*********************************************************************************************************
*                                            SMTPc_SetMbox()
*
* Description : (1) Populates a SMTPc_MBOX structure with associated name and address.
*
*                   (a) Perform error checking.
*                   (b) Copy arguments in structure.
*
*
* Argument(s) : p_mbox          SMTPc_MBOX structure to be populated.
*               p_name          Name of the mailbox owner.
*               p_addr          Address associated with the mailbox.
*               p_err           Pointer to variable that will hold the return error code from this
*                               function :
*
*                               SMTPc_ERR_NONE                      No error, structure ready.
*                               SMTPc_ERR_NULL_ARG                  Argument 'mbox'/'name' passed a NULL pointer.
*                               SMTPc_ERR_STR_TOO_LONG              Argument addr too long.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : (2) The name of the mailbox owner is not mandatory.  Passing NULL will result in an
*                   empty string being copied in the structure.
*********************************************************************************************************
*/

void  SMTPc_SetMbox (SMTPc_MBOX  *p_mbox,
                     CPU_CHAR    *p_name,
                     CPU_CHAR    *p_addr,
                     SMTPc_ERR   *p_err)
{
    CPU_SIZE_T  len;


    if ((p_mbox == (SMTPc_MBOX *)0) ||
        (p_addr == (CPU_CHAR   *)0)) {
        *p_err = SMTPc_ERR_NULL_ARG;
         return;
    }

    len = Str_Len(p_addr);
    if (len >= SMTPc_MBOX_ADDR_LEN) {
        *p_err = SMTPc_ERR_STR_TOO_LONG;
         return;
    } else {
         Str_Copy(p_mbox->Addr, p_addr);
    }

    if (p_name == (CPU_CHAR *)0) {                                /* See Note #2.                                         */
        Str_Copy((CPU_CHAR *)p_mbox->NameDisp,
                 (CPU_CHAR *)"");
    } else {
        len = Str_Len(p_name);
        if (len >= SMTPc_CFG_MBOX_NAME_DISP_LEN) {
            *p_err = SMTPc_ERR_STR_TOO_LONG;
             return;
        } else {
             Str_Copy(p_mbox->NameDisp, p_name);
        }
    }

    *p_err = SMTPc_ERR_NONE;
}


/*
*********************************************************************************************************
*                                            SMTPc_SetMsg()
*
* Description : Sets the various fields of a SMTPc_MSG structure so that it is valid and usable.
*
* Argument(s) : p_msg           SMTPc_MSG structure to be initialized.
*               p_err           Pointer to variable that will hold the return error code from this
*                               function :
*
*                               SMTPc_ERR_NONE                      No error, structure ready.
*                               SMTPc_ERR_NULL_ARG                  Argument 'p_msg'/'from' passed a NULL pointer.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) This function MUST be called after declaring a SMTPc_MSG structure and BEFORE beginning
*                   to manipulate it.  Failure to do so will likely produce run-time errors.
*
*               (2) The SMTPc_MSG structure member 'MIMEMsgHdrStruct' is left uninitialized for now,
*                   MIME extensions not being supported in this version.
*********************************************************************************************************
*/

void  SMTPc_SetMsg (SMTPc_MSG  *p_msg,
                    SMTPc_ERR  *p_err)
{
    CPU_INT08U  i;


    if (p_msg  == (SMTPc_MSG *)0) {
       *p_err  = SMTPc_ERR_NULL_ARG;
        return;
    }

    p_msg->From   = (SMTPc_MBOX *)0;
    p_msg->Sender = (SMTPc_MBOX *)0;

                                                                /* Set ptr's to NULL or 0                               */
    for (i = 0; i < SMTPc_CFG_MSG_MAX_TO; i++) {
        p_msg->ToArray[i] = (SMTPc_MBOX *)0;
    }

    for (i = 0; i < SMTPc_CFG_MSG_MAX_CC; i++) {
        p_msg->CCArray[i] = (SMTPc_MBOX *)0;
    }

    for (i = 0; i < SMTPc_CFG_MSG_MAX_BCC; i++) {
        p_msg->BCCArray[i] = (SMTPc_MBOX *)0;
    }

    for (i = 0; i < SMTPc_CFG_MSG_MAX_ATTACH; i++) {
        p_msg->AttachArray[i] = (SMTPc_ATTACH *)0;
    }

    p_msg->ReplyTo           = (SMTPc_MBOX *)0;
    p_msg->ContentBodyMsg    = (CPU_CHAR   *)0;
    p_msg->ContentBodyMsgLen = 0;
    p_msg->Subject           = DEF_NULL;
                                                                /* Clr CPU_CHAR arrays                                  */
    Mem_Clr((void     *)p_msg->MsgID,
            (CPU_SIZE_T)SMTPc_MIME_ID_LEN);

    SMTPc_TRACE_DBG(("SMTPc_SetMsg() success\n\r"));


   *p_err = SMTPc_ERR_NONE;
}


/*
*********************************************************************************************************
*                                            SMTPc_SendMail()
*
* Description : Send an email.
*
* Argument(s) : p_host_name       Pointer to host name of the SMTP server to contact. Can be also an IP address.
*
*               port              TCP port to use, or '0' if SMTPc_DFLT_PORT.
*
*               p_username        Pointer to user name, if authentication enabled.
*
*               p_pwd             Pointer to password,  if authentication enabled.
*
*               p_secure_cfg      Pointer to the secure configuration (TLS/SSL):
*
*                                       DEF_NULL, if no security enabled.
*                                       Pointer to a structure that contains the parameters.
*
*               p_msg               SMTPc_MSG structure encapsulating the message to send.
*
*               p_err             Pointer to variable that will hold the return error from this
*                                 function:
*
*                                 SMTPc_ERR_NONE                      No error, TCP connection established.
*                                 SMTPc_ERR_INVALID_ADDR              Argument server_addr_ascii and/or client_addr_ascii invalid
*
*                                                                   ------ RETURNED BY SMTPc_Connect : ------
*                                 SMTPc_ERR_NULL_ARG                  Argument 'username'/'pw' passed a NULL pointer.
*                                 SMTPc_ERR_SOCK_OPEN_FAILED          Error opening socket.
*                                 SMTPc_ERR_SOCK_CONN_FAILED          Error connecting to server.
*                                 SMTPc_ERR_RX_FAILED                 Error receiving server reply.
*                                 SMTPc_ERR_REP                       Error with reply.
*                                 SMTPc_ERR_SECURE_NOT_AVAIL          No secure mode available.
*                                 SMTPc_ERR_ENCODE                    Error encoding credentials.
*                                 SMTPc_ERR_AUTH_FAILED               Error authenticating user.
*
*                                                                   ------- RETURNED BY SMTPc_SendMsg : ------
*                                 SMTPc_ERR_NULL_ARG                  Argument 'p_msg' passed a NULL pointer.
*                                 SMTPc_ERR_RX_FAILED                 Error receiving server reply.
*                                 SMTPc_ERR_REP                       Error with reply.
*                                 SMTPc_ERR_TX_FAILED                 Error querying server.
*                                 SMTPc_ERR_RX_FAILED                 Error receiving server reply.
*                                 SMTPc_ERR_REP                       Error with reply.
*                                 SMTPc_ERR_TX_FAILED                 Error querying server.
*                                 SMTPc_ERR_LINE_TOO_LONG             Line limit exceeded.
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  SMTPc_SendMail (CPU_CHAR                *p_host_name,
                      CPU_INT16U               port,
                      CPU_CHAR                *p_username,
                      CPU_CHAR                *p_pwd,
                      NET_APP_SOCK_SECURE_CFG *p_secure_cfg,
                      SMTPc_MSG               *p_msg,
                      SMTPc_ERR               *p_err)
{
    NET_SOCK_ID  sock;

                                                                /* -------------- CONNECT TO SMTP SEVER --------------- */
    sock = SMTPc_Connect(p_host_name,
                         port,
                         p_username,
                         p_pwd,
                         p_secure_cfg,
                         p_err);
    if (*p_err != SMTPc_ERR_NONE) {
         return;
    }
                                                                /* ----------------- SEND THE MESSAGE ----------------- */
    SMTPc_SendMsg(sock, p_msg, p_err);
    if (*p_err != SMTPc_ERR_NONE) {
         return;
    }
                                                                /* ----------- DISCONNECT FROM SMTP SERVER ------------ */
    SMTPc_Disconnect(sock, p_err);
}


/*
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            SMTPc_RxReply()
*
* Description : Receive and process reply from the SMTP server.
*
* Argument(s) : sock_id         Socket ID.
*               perr            Pointer to variable that will hold the return error code from this
*                               function :
*
*                               SMTPc_ERR_NONE                      No error.
*                               SMTPc_ERR_RX_FAILED                 Error receiving the reply.
*
* Return(s)   : Complete reply from the server, if NO reception error.
*
*               (CPU_CHAR *)0,                  otherwise.
*
* Caller(s)   : SMTPc_Connect(),
*               SMTPc_SendMsg(),
*               SMTPc_HELO(),
*               SMTPc_MAIL(),
*               SMTPc_RCPT(),
*               SMTPc_DATA(),
*               SMTPc_RSET(),
*               SMTPc_QUIT().
*
* Note(s)     : (1) Server reply is at least 3 characters long (3 digits), plus CRLF.  Hence, receiving
*                   less than that automatically indicates an error.
*********************************************************************************************************
*/

static  CPU_CHAR  *SMTPc_RxReply (NET_SOCK_ID   sock_id,
                                  SMTPc_ERR    *perr)
{
    NET_SOCK_RTN_CODE  rx_len;
    NET_ERR            err;


                                                                /* ---------------------- RX REPLY -------------------- */
    rx_len = NetSock_RxData(sock_id,
                            SMTPc_Comm_Buf,
                            SMTPc_COMM_BUF_LEN - 1,
                            NET_SOCK_FLAG_NONE,
                           &err);

                                                                /* See Note #1.                                         */
    if ((rx_len == NET_SOCK_BSD_ERR_RX) ||
        (rx_len <= 4                  )) {
        *perr = SMTPc_ERR_RX_FAILED;
         return ((CPU_CHAR *)0);
    }

     SMTPc_Comm_Buf[rx_len] = '\0';

    *perr = SMTPc_ERR_NONE;

     return (SMTPc_Comm_Buf);
}


/*
*********************************************************************************************************
*                                          SMTPc_ParseReply()
*
* Description : (1) Process reply received from the SMTP server.
*
*                   (a) Parse reply
*                   (b) Interpret reply
*
*
* Argument(s) : server_reply    Complete reply received from the server.
*               completion_code Numeric value returned by server indicating command status.
*               perr            Pointer to variable that will hold the return error code from this
*                               function :
*
*                               SMTPc_ERR_REP_TOO_SHORT             Reply not long enough.
*                               SMTPc_ERR_REP_POS                   No error, positive reply received.
*                               SMTPc_ERR_REP_INTER                 No error, intermediate reply received.
*                               SMTPc_ERR_REP_NEG                   Negative reply received.
*
* Return(s)   : none.
*
* Caller(s)   : SMTPc_Connect(),
*               SMTPc_SendMsg(),
*               SMTPc_HELO(),
*               SMTPc_MAIL(),
*               SMTPc_RCPT(),
*               SMTPc_DATA(),
*               SMTPc_RSET(),
*               SMTPc_QUIT().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  SMTPc_ParseReply (CPU_CHAR    *server_reply,
                                CPU_INT32U  *completion_code,
                                SMTPc_ERR   *perr)
{
    CPU_INT08U  code_first_dig;
    CPU_INT08U  len;

                                                                /* -------------------- PARSE REPLY  ------------------ */
    len = Str_Len(server_reply);                                /* Make sure string is at least 3 + 1 char long.        */
    if (len < 4) {
        *perr = SMTPc_ERR_REP_TOO_SHORT;
         return;
    }
    
   *completion_code = Str_ParseNbr_Int32U(server_reply, DEF_NULL, 10);   
    SMTPc_TRACE_DBG(("Code: %u\n\r", (unsigned int)*completion_code));
    
    
                                                                /* ------------------ INTERPRET REPLY ----------------- */
    code_first_dig = *completion_code / 100;
    switch (code_first_dig) {
        case SMTPc_REP_POS_COMPLET_GRP:                         /* Positive reply.                                      */
            *perr = SMTPc_ERR_REP_POS;
             break;

        case SMTPc_REP_POS_PRELIM_GRP:                          /* Intermediate reply.                                  */
        case SMTPc_REP_POS_INTER_GRP:
            *perr = SMTPc_ERR_REP_INTER;
             break;

        case SMTPc_REP_NEG_TRANS_COMPLET_GRP:                   /* Negative reply.                                      */
        case SMTPc_REP_NEG_COMPLET_GRP:
            *perr = SMTPc_ERR_REP_NEG;
             break;

        default:                                                /* Should never happen, interpreted as negative.        */
            *perr = SMTPc_ERR_REP_NEG;
             break;
    }
}


/*
*********************************************************************************************************
*                                           SMTPc_QueryServer()
*
* Description : Send a query (or anything else) to the server.
*
* Argument(s) : sock_id         Socket ID.
*               query           Query in question.
*               len             Length of message to transmit
*               perr            Pointer to variable that will hold the return error code from this
*                               function :
*
*                               SMTPc_ERR_NONE                      No error.
*
*                                                                   ----- RETURNED BY NetSock_TxData() : -----
*                                                                   See uC/TCPIP source code.
*
* Return(s)   : none.
*
* Caller(s)   : SMTPc_SendBody(),
*               SMTPc_BuildHdr(),
*               SMTPc_HELO(),
*               SMTPc_MAIL(),
*               SMTPc_RCPT(),
*               SMTPc_DATA(),
*               SMTPc_RSET(),
*               SMTPc_QUIT().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  SMTPc_QueryServer (NET_SOCK_ID   sock_id,
                                 CPU_CHAR     *query,
                                 CPU_INT32U    len,
                                 SMTPc_ERR    *perr)
{
    NET_SOCK_RTN_CODE  rtn_code;
    CPU_INT32U         cur_pos;
    NET_ERR            err;

                                                                /* ---------------------- TX QUERY -------------------- */
    cur_pos = 0;

    do {
        rtn_code = NetSock_TxData( sock_id,
                                  &query[cur_pos],
                                   len,
                                   0,
                                  &err);

        cur_pos  = cur_pos + rtn_code;
        len      = len - rtn_code;

    } while ((len != 0) && (rtn_code != NET_SOCK_BSD_ERR_TX));

    if (rtn_code != NET_SOCK_BSD_ERR_TX) {
       *perr = SMTPc_ERR_NONE;
    }
}


/*
*********************************************************************************************************
*                                           SMTPc_SendBody()
*
* Description : (1) Prepare and send the actual data of the message, i.e. the body part of the message
*                   content.
*
*                   (a) Built headers and transmit
*                   (b) Transmit body content
*                   (c) Prepare and transmit attachment(s)
*                   (d) Transmit "end of mail data" indicator
*                   (e) Receive the confirmation reply
*
*
* Argument(s) : sock_id         Socket ID.
*               msg             SMTPc_MSG structure encapsulating the message to send.
*               perr            Pointer to variable that will hold the return error code from this
*                               function :
*
*                               SMTPc_ERR_NONE                      No error.
*                               SMTPc_ERR_TX_FAILED                 Error querying server.
*
*                                                                   ----- RETURNED BY SMTPc_BuildHdr() : -----
*                               SMTPc_ERR_NONE                      No error.
*                               SMTPc_ERR_LINE_TOO_LONG             Line limit exceeded.
*                               SMTPc_ERR_TX_FAILED                 Error querying server.
*
* Return(s)   : none.
*
* Caller(s)   : SMTPc_SendMsg().
*
* Note(s)     : (2) The current implementation does not insert the names of the mailbox owners (member
*                   NameDisp of structure SMTPc_MBOX).
*********************************************************************************************************
*/

static  void  SMTPc_SendBody (NET_SOCK_ID   sock_id,
                              SMTPc_MSG    *msg,
                              SMTPc_ERR    *perr)
{
    CPU_SIZE_T   len;
    CPU_INT32U   cur_wr_ix;
    CPU_INT08U   i;
    CPU_INT32U   line_len;
    CPU_CHAR    *hdr;
    CPU_CHAR    *reply;
    CPU_INT32U   completion_code;


    cur_wr_ix = 0;
    line_len  = 0;

                                                                /* ------------------- BUILT HEADERS ------------------ */
                                                                /* Header "From: ".                                     */
    hdr       = (CPU_CHAR *)SMTPc_HDR_FROM;
    cur_wr_ix = SMTPc_BuildHdr( sock_id,                        /* Addr                                                 */
                                SMTPc_Comm_Buf,
                                SMTPc_COMM_BUF_LEN,
                                cur_wr_ix,
                                hdr,
                                msg->From->Addr,
                               &line_len,
                                perr);
    if (*perr != SMTPc_ERR_NONE) {
        return;
    }


    if (msg->Sender != (SMTPc_MBOX *)0) {                       /* Header "Sender: ".                                   */
        hdr       = (CPU_CHAR *)SMTPc_HDR_SENDER;
        cur_wr_ix = SMTPc_BuildHdr( sock_id,                    /* Addr                                                 */
                                    SMTPc_Comm_Buf,
                                    SMTPc_COMM_BUF_LEN,
                                    cur_wr_ix,
                                    hdr,
                                    msg->Sender->Addr,
                                   &line_len,
                                    perr);
        if (*perr != SMTPc_ERR_NONE) {
            return;
        }
    }



                                                                /* Header "To: ".                                       */
    hdr = (CPU_CHAR *)SMTPc_HDR_TO;
    for (i = 0; (i < SMTPc_CFG_MSG_MAX_TO) && (msg->ToArray[i] != (SMTPc_MBOX *)0); i++) {
        cur_wr_ix = SMTPc_BuildHdr( sock_id,
                                    SMTPc_Comm_Buf,
                                    SMTPc_COMM_BUF_LEN,
                                    cur_wr_ix,
                                    hdr,
                                    msg->ToArray[i]->Addr,
                                   &line_len,
                                    perr);
        if (*perr != SMTPc_ERR_NONE) {
            return;
        }
        hdr = (CPU_CHAR *)0;
    }


                                                                /* Header "Reply-to: ".                                 */
    hdr = (CPU_CHAR *)SMTPc_HDR_REPLYTO;
    if (msg->ReplyTo != (SMTPc_MBOX *)0) {
        cur_wr_ix = SMTPc_BuildHdr( sock_id,
                                    SMTPc_Comm_Buf,
                                    SMTPc_COMM_BUF_LEN,
                                    cur_wr_ix,
                                    hdr,
                                    msg->ReplyTo->Addr,
                                   &line_len,
                                    perr);
        if (*perr != SMTPc_ERR_NONE) {
            return;
        }
        hdr = (CPU_CHAR *)0;
    }


                                                                /* Header "CC: ".                                       */
    hdr = (CPU_CHAR *)SMTPc_HDR_CC;
    for (i = 0; (i < SMTPc_CFG_MSG_MAX_CC) && (msg->CCArray[i] != (SMTPc_MBOX *)0); i++) {
        cur_wr_ix = SMTPc_BuildHdr( sock_id,
                                    SMTPc_Comm_Buf,
                                    SMTPc_COMM_BUF_LEN,
                                    cur_wr_ix,
                                    hdr,
                                    msg->CCArray[i]->Addr,
                                   &line_len,
                                    perr);
        if (*perr != SMTPc_ERR_NONE) {
            return;
        }
        hdr = (CPU_CHAR *)0;
    }


    if (msg->Subject != (CPU_CHAR *)0) {                        /* Header "Subject: ".                                  */
        cur_wr_ix = SMTPc_BuildHdr((NET_SOCK_ID ) sock_id,
                                   (CPU_CHAR   *) SMTPc_Comm_Buf,
                                   (CPU_INT32U  ) SMTPc_COMM_BUF_LEN,
                                   (CPU_INT32U  ) cur_wr_ix,
                                   (CPU_CHAR   *) SMTPc_HDR_SUBJECT,
                                   (CPU_CHAR   *) msg->Subject,
                                   (CPU_INT32U *)&line_len,
                                   (SMTPc_ERR  *) perr);
        if (*perr != SMTPc_ERR_NONE) {
            return;
        }
    }
                                                                /* ----------- INSERT HEADER/BODY DELIMITER ----------- */
    Mem_Copy(&SMTPc_Comm_Buf[cur_wr_ix], SMTPc_CRLF, SMTPc_CRLF_SIZE);
    cur_wr_ix += SMTPc_CRLF_SIZE;

                                                                /* ---------------- TX CONTENT HEADERS ---------------- */
    SMTPc_QueryServer(sock_id, SMTPc_Comm_Buf, cur_wr_ix, perr);
    if (*perr != SMTPc_ERR_NONE) {
       *perr = SMTPc_ERR_TX_FAILED;
        return;
    }


                                                                /* ------------------ TX BODY CONTENT ----------------- */
    SMTPc_QueryServer(sock_id, msg->ContentBodyMsg, msg->ContentBodyMsgLen, perr);
    if (*perr != SMTPc_ERR_NONE) {
       *perr = SMTPc_ERR_TX_FAILED;
        return;
    }



                                                                /* ------------ PREPARE & TX ATTACHMENT(S) ------------ */
                                                                /* #### Not currently implemented.                      */



                                                                /* ----------- TX END OF MAIL DATA INDICATOR ---------- */
    Mem_Copy(SMTPc_Comm_Buf, SMTPc_EOM, sizeof(SMTPc_EOM));
    len = Str_Len(SMTPc_Comm_Buf);
    SMTPc_QueryServer(sock_id, SMTPc_Comm_Buf, len, perr);
    if (*perr != SMTPc_ERR_NONE) {
       *perr = SMTPc_ERR_TX_FAILED;
        return;
    }

                                                                /* --------------- RX CONFIRMATION REPLY -------------- */
    reply = SMTPc_RxReply(sock_id, perr);
    if (*perr != SMTPc_ERR_NONE) {
       *perr = SMTPc_ERR_RX_FAILED;
        return;
    }

    SMTPc_ParseReply(reply, &completion_code, perr);
    switch (*perr) {
        case SMTPc_ERR_REP_POS:
             if (completion_code == SMTPc_REP_250) {
                *perr = SMTPc_ERR_NONE;
             }
             break;

        default:
            *perr = SMTPc_ERR_REP;
             break;
    }

    if (*perr != SMTPc_ERR_NONE) {
        if ((completion_code != SMTPc_REP_421) &&
            (completion_code != SMTPc_REP_221)) {
            SMTPc_RSET(sock_id, &completion_code, perr);
        }
       *perr = SMTPc_ERR_REP;
    }
}


/*
*********************************************************************************************************
*                                           SMTPc_BuildHdr()
*
* Description : (1) Prepare (and send if necessary) the message content's headers.
*
*                   (a) Calculate needed space
*                   (b) Send data, if necessary
*                   (c) Build header
*
*
* Argument(s) : sock_id         Socket ID.
*               buf             Buffer used to store the headers prior to their expedition.
*               buf_size        Size of buffer.
*               buf_wr_ix       Index of current "write" position.
*               hdr             Header name.
*               val             Value associated with header.
*               line_len        Current line total length.
*               perr            Pointer to variable that will hold the return error code from this
*                               function :
*
*                               SMTPc_ERR_NONE                      No error.
*                               SMTPc_ERR_LINE_TOO_LONG             Line limit exceeded.
*                               SMTPc_ERR_TX_FAILED                 Error querying server.
*
* Return(s)   : "Write" position in buffer.
*
* Caller(s)   : SMTPc_SendBody().
*
* Note(s)     : (2) If the parameter "hdr" is (CPU_CHAR *)0, et means that it's already been passed in a
*                   previous call.  Hence, a "," will be inserted in the buffer prior to the value.
*
*               (3) If the SMTP line limit is exceeded, perr is set to SMTPc_ERR_LINE_TOO_LONG and the
*                   function returns without having added the header.
*
*               (4) This implementation transmit the headers buffer if the next header is too large to
*                   be inserted in the remaining buffer space.
*
*                   Note that NO EXACT calculations are performed here;  a conservative approach is
*                   brought forward, without actually optimizing the process (i.e. a buffer could be
*                   sent even though a few more characters could have been inserted).
*
*               (5) CRLF is inserted even though more entries are still to come for a particular header
*                   (line folding is performed even if unnecessary).
*********************************************************************************************************
*/

static  CPU_INT32U  SMTPc_BuildHdr (NET_SOCK_ID   sock_id,
                                    CPU_CHAR     *buf,
                                    CPU_INT32U    buf_size,
                                    CPU_INT32U    buf_wr_ix,
                                    CPU_CHAR     *hdr,
                                    CPU_CHAR     *val,
                                    CPU_INT32U   *line_len,
                                    SMTPc_ERR    *perr)
{
    CPU_INT32U  hdr_len;
    CPU_INT32U  val_len;
    CPU_INT32U  total_len;

                                                                /* ------------- CALCULATE NECESSARY SPACE ------------ */
    if (hdr == (CPU_CHAR *)0) {
        hdr_len = 0;
    } else {
        hdr_len = Str_Len(hdr);
    }

    if (val == (CPU_CHAR *)0) {
        val_len = 0;
    } else {
        val_len = Str_Len(val);
    }

    total_len = hdr_len + val_len + 2;

    if ((*line_len + total_len) > SMTPc_LINE_LEN_LIM) {         /* See Note #3.                                         */
       *perr = SMTPc_ERR_LINE_TOO_LONG;
        return (buf_wr_ix);
    }



                                                                /* -------------- SEND DATA, IF NECESSARY ------------- */
                                                                /* See Note #4.                                         */
    if ((buf_size - buf_wr_ix) < total_len) {
        SMTPc_QueryServer(sock_id, buf, buf_wr_ix, perr);
        if (*perr != SMTPc_ERR_NONE) {
           *perr = SMTPc_ERR_TX_FAILED;
            return buf_wr_ix;
        }
        buf_wr_ix = 0;
    }


                                                                /* ------------------ BUILDING HEADER ----------------- */
    if (hdr != (CPU_CHAR *)0) {
        Mem_Copy(buf + buf_wr_ix, hdr, hdr_len);
        buf_wr_ix += hdr_len;
       *line_len += hdr_len;
    } else {                                                    /* Not first item, adding ','.                          */
        Mem_Copy(buf + buf_wr_ix, " ,", 2);
        buf_wr_ix += 2;
       *line_len += 2;
    }
    if (val != (CPU_CHAR *)0) {
        Mem_Copy(buf + buf_wr_ix, val, val_len);
        buf_wr_ix += val_len;
       *line_len += val_len;
    }

    Mem_Copy(buf + buf_wr_ix, SMTPc_CRLF, 2);                   /* See Note #5.                                         */
    buf_wr_ix += 2;
   *line_len  = 0;


    buf[buf_wr_ix] = '\0';
    SMTPc_TRACE_DBG(("String: %s\n", buf));

   *perr = SMTPc_ERR_NONE;
    return (buf_wr_ix);
}


/*
*********************************************************************************************************
*                                             SMTPc_HELO()
*
* Description : (1) Build the HELO command, send it to the server and validate reply.
*
*                   (a) Send command to the server
*                   (b) Receive server's reply and validate
*
*
* Argument(s) : sock_id         Socket ID.
*
*               completion_code Numeric value returned by server indicating command status.
*
*               perr            Pointer to variable that will hold the return error code from this
*                               function :
*
*                               SMTPc_ERR_NONE                      No error.
*                               SMTPc_ERR_TX_FAILED                 Error querying server.
*                               SMTPc_ERR_RX_FAILED                 Error receiving server reply.
*                               SMTPc_ERR_REP                       Error with reply.
*
* Return(s)   : Complete reply from the server, if NO reception error.
*
*               (CPU_CHAR *)0,                  otherwise.
*
* Caller(s)   : SMTPc_Connect().
*
* Note(s)     : (2) From RFC #2821, "the HELO command is used to identify the SMTP client to the SMTP
*                   server".
*
*               (3) The server will send a 250 "Requested mail action okay, completed" reply upon
*                   success.  A positive reply is the only reply that will lead to a  "SMTPc_ERR_NONE"
*                   error return code.
*
*               (4) This implementation will accept reply 250, as well as any other positive reply.
*
*               (5) From RFC #2821, section 4.1.3 Address Literals:
*
*                      For IPv4 addresses, this form uses four small decimal integers separated
*                      by dots and enclosed by brackets such as [123.255.37.2], which
*                      indicates an (IPv4) Internet Address in sequence-of-octets form.  For
*                      IPv6 and other forms of addressing that might eventually be
*                      standardized, the form consists of a standardized "tag" that
*                      identifies the address syntax, a colon, and the address itself, in a
*                      format specified as part of the IPv6 standards.
*
*
*********************************************************************************************************
*/

static  CPU_CHAR  *SMTPc_HELO (NET_SOCK_ID   sock_id,
                               CPU_INT32U   *completion_code,
                               SMTPc_ERR    *perr)
{
    CPU_CHAR        *reply;
    CPU_SIZE_T       len;
    CPU_INT08U       client_addr[NET_CONN_ADDR_LEN_MAX];
    NET_SOCK_FAMILY  client_addr_family;
    NET_ERR          err_net;
    CPU_CHAR         client_addr_ascii[NET_ASCII_LEN_MAX_ADDR_IP];
    NET_IPv4_ADDR    ipv4_client_addr;

                                                                /* Get the IP address used in the conn.                 */
    NetSock_GetLocalIPAddr(sock_id,
                           client_addr,
                          &client_addr_family,
                          &err_net);
    if (err_net != NET_SOCK_ERR_NONE) {
       *perr = SMTPc_ERR_TX_FAILED;
        return ((CPU_CHAR *)0);
    }
                                                                /* Format the message depending of the address family.  */

    switch (client_addr_family) {
#ifdef  NET_IPv4_MODULE_EN
        case NET_SOCK_FAMILY_IP_V4:

             ipv4_client_addr = NET_UTIL_NET_TO_HOST_32(*(CPU_INT32U *)client_addr);
             NetASCII_IPv4_to_Str(ipv4_client_addr, client_addr_ascii, DEF_NO, &err_net);

             if (err_net != NET_ASCII_ERR_NONE) {
                *perr = SMTPc_ERR_TX_FAILED;
                 return ((CPU_CHAR *)0);
             }

              Str_Copy(SMTPc_Comm_Buf, SMTPc_CMD_HELO);
              Str_Cat(SMTPc_Comm_Buf," [");
              Str_Cat(SMTPc_Comm_Buf,client_addr_ascii);
              Str_Cat(SMTPc_Comm_Buf,"]\r\n");
              break;
#endif

#ifdef  NET_IPv6_MODULE_EN
        case NET_SOCK_FAMILY_IP_V6:

             NetASCII_IPv6_to_Str((NET_IPv6_ADDR *) client_addr,
                                                    client_addr_ascii,
                                                    DEF_NO,
                                                    DEF_NO,
                                                   &err_net);
             if (err_net != NET_ASCII_ERR_NONE) {
                *perr = SMTPc_ERR_TX_FAILED;
                 return ((CPU_CHAR *)0);
             }

             Str_Copy(SMTPc_Comm_Buf, SMTPc_CMD_HELO);
             Str_Cat(SMTPc_Comm_Buf, " [");
             Str_Cat(SMTPc_Comm_Buf, SMTPc_TAG_IPv6);
             Str_Cat(SMTPc_Comm_Buf, " ");
             Str_Cat(SMTPc_Comm_Buf, client_addr_ascii);
             Str_Cat(SMTPc_Comm_Buf, "]\r\n");
             break;
#endif
         default:
            *perr = SMTPc_ERR_TX_FAILED;
             return ((CPU_CHAR *)0);
    }

    len = Str_Len(SMTPc_Comm_Buf);
                                                                /* Send HELO Query.                                     */
    SMTPc_QueryServer(sock_id, SMTPc_Comm_Buf, len, perr);
    if (*perr != SMTPc_ERR_NONE) {
       *perr  = SMTPc_ERR_TX_FAILED;
        return ((CPU_CHAR *)0);
    }

                                                                /* --------- RX SERVER'S REPLY & VALIDATE ------------- */
    reply = SMTPc_RxReply(sock_id, perr);                       /* See Note #3.                                         */
    if (*perr != SMTPc_ERR_NONE) {
       *perr  = SMTPc_ERR_RX_FAILED;
        return ((CPU_CHAR *)0);
    }

    SMTPc_ParseReply(reply, completion_code, perr);             /* See Note #4.                                         */
    switch (*perr) {
        case SMTPc_ERR_REP_POS:                                 /* If any pos rep...                                    */
            *perr = SMTPc_ERR_NONE;                             /* ... no err.                                          */
             break;

        case SMTPc_ERR_REP_TOO_SHORT:
            *perr = SMTPc_ERR_REP;
             reply = (CPU_CHAR *)0;
             break;

        default:
            *perr = SMTPc_ERR_REP;
             break;
    }

    return (reply);
}


/*
*********************************************************************************************************
*                                            SMTPc_AUTH()
*
* Description : (1) Build the AUTH command, send it to the server and validate reply.
*
*                   (a) Encode username & password
*                   (b) Send command to the server
*                   (c) Receive server's reply and validate
*
*
* Argument(s) : sock_id          Socket ID.
*               username         Mailbox username name for authentication.
*               pw               Mailbox password      for authentication.
*               completion_code  Numeric value returned by server indicating command status.
*               perr             Pointer to variable that will hold the return error code from this
*                                function :
*
*                                SMTPc_ERR_NONE                     No error.
*                                SMTPc_ERR_TX_FAILED                Error querying server.
*                                SMTPc_ERR_RX_FAILED                Error receiving server reply.
*                                SMTPc_ERR_REP                      Error with reply.
*                                SMTPc_ERR_ENCODE                   Error encoding credentials.
*                                SMTPc_ERR_AUTH_FAILED              Error authenticating user.
*
* Return(s)   : Complete reply from the server, if NO reception error.
*
*               (CPU_CHAR *)0,                  otherwise.
*
* Caller(s)   : SMTPc_Connect().
*
* Note(s)     : (2) The user's credentials are transmitted to the server using a base 64 encoding and
*                   formated according to RFC #4616.  From Section 2 'PLAIN SALS Mechanism' "The client
*                   presents the authorization identity (identity to act as), followed by a NUL (U+0000)
*                   character, followed by the authentication identity (identity whose password will be
*                   used), followed by a NUL (U+0000) character, followed by the clear-text password.
*                   As with other SASL mechanisms, the client does not provide an authorization identity
*                   when it wishes the server to derive an identity from the credentials and use that
*                   as the authorization identity."
*
*               (3) The server will send a 235 "Authentication successful" reply upon success.  A positive
*                   reply is the only reply that will lead to a  "SMTPc_ERR_NONE" error return code.
*
*               (4) This implementation will accept reply 235, as well as any other positive reply.
*********************************************************************************************************
*/

#if (SMTPc_CFG_AUTH_EN == DEF_ENABLED)
static  CPU_CHAR  *SMTPc_AUTH (NET_SOCK_ID   sock_id,
                               CPU_CHAR     *username,
                               CPU_CHAR     *pw,
                               CPU_INT32U   *completion_code,
                               SMTPc_ERR    *perr)
{
    CPU_CHAR     unencoded_buf[SMTPc_ENCODER_BASE54_IN_MAX_LEN];
    CPU_CHAR     encoded_buf[SMTPc_ENCODER_BASE64_OUT_MAX_LEN];
    CPU_INT16U   unencoded_len;
    CPU_CHAR    *reply;
    CPU_INT16U   wr_ix;
    CPU_SIZE_T   len;
    NET_ERR      net_err;

                                                                /* -------------- ENCODE USERNAME & PW ---------------- */
                                                                /* See Note #2.                                         */
    wr_ix = 0;
    unencoded_buf[wr_ix] = SMTPc_ENCODER_BASE64_DELIMITER_CHAR;
    wr_ix++;

    Str_Copy(&unencoded_buf[wr_ix], username);
    wr_ix += Str_Len(username);

    unencoded_buf[wr_ix] = SMTPc_ENCODER_BASE64_DELIMITER_CHAR;
    wr_ix++;

    Str_Copy(&unencoded_buf[wr_ix], pw);
    unencoded_len = wr_ix + Str_Len(pw);

                                                                /* Base 64 encoding.                                    */
    NetBase64_Encode (unencoded_buf, unencoded_len, encoded_buf, SMTPc_ENCODER_BASE64_OUT_MAX_LEN, &net_err);
    if (net_err != NET_ERR_NONE) {
       *perr = SMTPc_ERR_ENCODE;
        return ((CPU_CHAR *)0);
    }

                                                                /* ------------------ TX CMD TO SERVER ---------------- */
    Str_Copy(SMTPc_Comm_Buf, SMTPc_CMD_AUTH);
    Str_Cat(SMTPc_Comm_Buf, " ");
    Str_Cat(SMTPc_Comm_Buf, SMTPc_CMD_AUTH_MECHANISM_PLAIN);
    Str_Cat(SMTPc_Comm_Buf, " ");
    Str_Cat(SMTPc_Comm_Buf, encoded_buf);
    Str_Cat(SMTPc_Comm_Buf, "\r\n");
    len = Str_Len(SMTPc_Comm_Buf);

    SMTPc_QueryServer(sock_id, SMTPc_Comm_Buf, len, perr);
    if (*perr != SMTPc_ERR_NONE) {
       *perr  = SMTPc_ERR_TX_FAILED;
        return ((CPU_CHAR *)0);
    }

                                                                /* ---------- RX SERVER'S RESPONSE & VALIDATE --------- */
    reply = SMTPc_RxReply(sock_id, perr);                       /* See Note #3.                                         */
    if (*perr != SMTPc_ERR_NONE) {
       *perr  = SMTPc_ERR_RX_FAILED;
        return ((CPU_CHAR *)0);
    }


    SMTPc_ParseReply(reply, completion_code, perr);             /* See Note #4.                                         */
    switch (*perr) {
        case SMTPc_ERR_REP_POS:                                 /* If any pos rep...                                    */
            *perr = SMTPc_ERR_NONE;                             /* ... no err.                                          */
             break;

        case SMTPc_ERR_REP_NEG:
             if (*completion_code == SMTPc_REP_535) {
                *perr = SMTPc_ERR_AUTH_FAILED;
             } else {
                *perr = SMTPc_ERR_REP;
             }
             break;

        case SMTPc_ERR_REP_TOO_SHORT:
            *perr = SMTPc_ERR_REP;
             reply = (CPU_CHAR *)0;
             break;

        default:
            *perr = SMTPc_ERR_REP;
             break;
    }

    return (reply);
}
#endif

/*
*********************************************************************************************************
*                                             SMTPc_MAIL()
*
* Description : (1) Build the MAIL command, send it to the server and validate reply.
*
*                   (a) Send command to the server
*                   (b) Receive server's reply and validate
*
*
* Argument(s) : sock_id         Socket ID.
*               from            Argument of the "MAIL" command (sender mailbox).
*               completion_code Numeric value returned by server indicating command status.
*               perr            Pointer to variable that will hold the return error code from this
*                               function :
*
*                               SMTPc_ERR_NONE                      No error.
*                               SMTPc_ERR_TX_FAILED                 Error querying server.
*                               SMTPc_ERR_RX_FAILED                 Error receiving server reply.
*                               SMTPc_ERR_REP                       Error with reply.
*
* Return(s)   : Complete reply from the server, if NO reception error.
*
*               (CPU_CHAR *)0,                  otherwise.
*
* Caller(s)   : SMTPc_SendMsg().
*
* Note(s)     : (2) From RFC #2821, "the MAIL command is used to initiate a mail transaction in which
*                   the mail data is delivered to an SMTP server [...]".
*
*               (3) The server will send a 250 "Requested mail action okay, completed" reply upon
*                   success.  A positive reply is the only reply that will lead to a  "SMTPc_ERR_NONE"
*                   error return code.
*
*               (4) This implementation will accept reply 250, as well as any other positive reply.
*********************************************************************************************************
*/

static  CPU_CHAR  *SMTPc_MAIL (NET_SOCK_ID   sock_id,
                               CPU_CHAR     *from,
                               CPU_INT32U   *completion_code,
                               SMTPc_ERR    *perr)
{
    CPU_CHAR    *reply;
    CPU_SIZE_T   len;

                                                                /* ----------------- TX CMD TO SERVER ----------------- */
    Str_Copy(SMTPc_Comm_Buf, SMTPc_CMD_MAIL);
    Str_Cat(SMTPc_Comm_Buf, " FROM:<");
    Str_Cat(SMTPc_Comm_Buf, from);
    Str_Cat(SMTPc_Comm_Buf, ">\r\n");

    len = Str_Len(SMTPc_Comm_Buf);
    SMTPc_QueryServer(sock_id, SMTPc_Comm_Buf, len, perr);
    if (*perr != SMTPc_ERR_NONE) {
       *perr  = SMTPc_ERR_TX_FAILED;
        return ((CPU_CHAR *)0);
    }

                                                                /* ---------- RX SERVER'S RESPONSE & VALIDATE --------- */
    reply = SMTPc_RxReply(sock_id, perr);                       /* See Note #3.                                         */
    if (*perr != SMTPc_ERR_NONE) {
       *perr  = SMTPc_ERR_RX_FAILED;
        return ((CPU_CHAR *)0);
    }


    SMTPc_ParseReply(reply, completion_code, perr);             /* See Note #4.                                         */
    switch (*perr) {
        case SMTPc_ERR_REP_POS:                                 /* If any pos rep...                                    */
            *perr = SMTPc_ERR_NONE;                             /* ... no err.                                          */
             break;

        case SMTPc_ERR_REP_TOO_SHORT:
            *perr = SMTPc_ERR_REP;
             reply = (CPU_CHAR *)0;
             break;

        default:
            *perr = SMTPc_ERR_REP;
             break;
    }

    return (reply);
}


/*
*********************************************************************************************************
*                                             SMTPc_RCPT()
*
* Description : (1) Build the RCPT command, send it to the server and validate reply.
*
*                   (a) Send command to the server
*                   (b) Receive server's reply and validate
*
*
* Argument(s) : sock_id         Socket ID.
*               to              Argument of the "RCPT" command (receiver mailbox).
*               completion_code Numeric value returned by server indicating command status.
*               perr            Pointer to variable that will hold the return error code from this
*                               function :
*
*                               SMTPc_ERR_NONE                      No error.
*                               SMTPc_ERR_TX_FAILED                 Error querying server.
*                               SMTPc_ERR_RX_FAILED                 Error receiving server reply.
*                               SMTPc_ERR_REP                       Error with reply.
*
* Return(s)   : Complete reply from the server, if NO reception error.
*
*               (CPU_CHAR *)0,                  otherwise.
*
* Caller(s)   : SMTPc_SendMsg().
*
* Note(s)     : (2) From RFC #2821, "the RCPT command is used to identify an individual recipient of the
*                   mail data; multiple recipients are specified by multiple use of this command".
*
*               (3) The server will send a 250 "Requested mail action okay, completed"  or a 251 "User
*                   not local; will forwarded to <forward-path>" reply upon success.
*
*               (4) This implementation will accept replies 250 and 251 as positive replies.  Reply 551
*                   "User not local; please try <forward-path>" will result in an error.
*********************************************************************************************************
*/

static  CPU_CHAR  *SMTPc_RCPT (NET_SOCK_ID   sock_id,
                               CPU_CHAR     *to,
                               CPU_INT32U   *completion_code,
                               SMTPc_ERR    *perr)
{
    CPU_CHAR    *reply;
    CPU_SIZE_T   len;

                                                                /* ----------------- TX CMD TO SERVER ----------------- */
    Str_Copy(SMTPc_Comm_Buf, SMTPc_CMD_RCPT);
    Str_Cat(SMTPc_Comm_Buf, " TO:<");
    Str_Cat(SMTPc_Comm_Buf, to);
    Str_Cat(SMTPc_Comm_Buf, ">\r\n");

    len = Str_Len(SMTPc_Comm_Buf);
    SMTPc_QueryServer(sock_id, SMTPc_Comm_Buf, len, perr);
    if (*perr != SMTPc_ERR_NONE) {
       *perr  = SMTPc_ERR_TX_FAILED;
        return ((CPU_CHAR *)0);
    }

                                                                /* ---------- RX SERVER'S RESPONSE & VALIDATE --------- */
    reply = SMTPc_RxReply(sock_id, perr);                       /* See Note #3.                                         */
    if (*perr != SMTPc_ERR_NONE) {
       *perr  = SMTPc_ERR_RX_FAILED;
        return ((CPU_CHAR *)0);
    }

    SMTPc_ParseReply(reply, completion_code, perr);             /* See Note #4.                                         */
    switch (*perr) {
        case SMTPc_ERR_REP_POS:
             if ((*completion_code == SMTPc_REP_250) ||
                 (*completion_code == SMTPc_REP_251)) {
                *perr = SMTPc_ERR_NONE;
             } else {
                *perr = SMTPc_ERR_REP;
             }
             break;

        case SMTPc_ERR_REP_TOO_SHORT:
            *perr = SMTPc_ERR_REP;
             reply = (CPU_CHAR *)0;
             break;

        default:
            *perr = SMTPc_ERR_REP;
             break;
    }

    return (reply);
}


/*
*********************************************************************************************************
*                                             SMTPc_DATA()
*
* Description : (1) Build the DATA command, send it to the server and validate reply.
*
*                   (a) Send command to the server
*                   (b) Receive server's reply and validate
*
*
* Argument(s) : sock_id         Socket ID.
*               completion_code Numeric value returned by server indicating command status.
*               perr            Pointer to variable that will hold the return error code from this
*                               function :
*
*                               SMTPc_ERR_NONE                      No error.
*                               SMTPc_ERR_TX_FAILED                 Error querying server.
*                               SMTPc_ERR_RX_FAILED                 Error receiving server reply.
*                               SMTPc_ERR_REP                       Error with reply.
*
* Return(s)   : Complete reply from the server, if NO reception error.
*
*               (CPU_CHAR *)0,                  otherwise.
*
* Caller(s)   : SMTPc_SendMsg().
*
* Note(s)     : (2) The DATA command is used to indicate to the SMTP server that all the following lines
*                   up to but not including the end of mail data indicator are to be considered as the
*                   message text.
*
*               (3) The receiver normally sends a 354 "Start mail input" reply and then treats the lines
*                   following the command as mail data from the sender.
*********************************************************************************************************
*/

static  CPU_CHAR  *SMTPc_DATA (NET_SOCK_ID   sock_id,
                               CPU_INT32U   *completion_code,
                               SMTPc_ERR    *perr)
{
    CPU_CHAR    *reply;
    CPU_SIZE_T   len;

                                                                /* ----------------- TX CMD TO SERVER ----------------- */
    Str_Copy(SMTPc_Comm_Buf, SMTPc_CMD_DATA);
    Str_Cat(SMTPc_Comm_Buf, "\r\n");


    len = Str_Len(SMTPc_Comm_Buf);
    SMTPc_QueryServer(sock_id, SMTPc_Comm_Buf, len, perr);
    if (*perr != SMTPc_ERR_NONE) {
       *perr  = SMTPc_ERR_TX_FAILED;
        return ((CPU_CHAR *)0);
    }

                                                                /* ---------- RX SERVER'S RESPONSE & VALIDATE --------- */
    reply = SMTPc_RxReply(sock_id, perr);                       /* See Note #3.                                         */
    if (*perr != SMTPc_ERR_NONE) {
       *perr  = SMTPc_ERR_RX_FAILED;
        return ((CPU_CHAR *)0);
    }

    SMTPc_ParseReply(reply, completion_code, perr);
    switch (*perr) {
        case SMTPc_ERR_REP_INTER:
             if (*completion_code == SMTPc_REP_354) {
                *perr = SMTPc_ERR_NONE;
             } else {
                *perr = SMTPc_ERR_REP;
             }
             break;

        case SMTPc_ERR_REP_TOO_SHORT:
            *perr = SMTPc_ERR_REP;
             reply = (CPU_CHAR *)0;
             break;

        default:
            *perr = SMTPc_ERR_REP;
             break;
    }

    return (reply);
}


/*
*********************************************************************************************************
*                                             SMTPc_RSET()
*
* Description : (1) Build the RSET command, send it to the server and validate reply.
*
*                   (a) Send command to the server
*                   (b) Receive server's reply and validate
*
*
* Argument(s) : sock_id         Socket ID.
*               completion_code Numeric value returned by server indicating command status.
*               perr            Pointer to variable that will hold the return error code from this
*                               function :
*
*                               SMTPc_ERR_NONE                      No error.
*                               SMTPc_ERR_TX_FAILED                 Error querying server.
*                               SMTPc_ERR_RX_FAILED                 Error receiving server reply.
*                               SMTPc_ERR_REP                       Error with reply.
*
* Return(s)   : Complete reply from the server, if NO reception error.
*
*               (CPU_CHAR *)0,                  otherwise.
*
* Caller(s)   : SMTPc_SendMsg().
*
* Note(s)     : (2) From RFC #2821, "the RSET command specifies that the current mail transaction will
*                   be aborted.  Any stored sender, recipients, and mail data MUST be discarded, and all
*                   buffers and state tables cleared".
*
*               (3) The server MUST send a 250 "Requested mail action okay, completed" reply to a RSET
*                   command with no arguments.
*********************************************************************************************************
*/

static  CPU_CHAR  *SMTPc_RSET (NET_SOCK_ID   sock_id,
                               CPU_INT32U   *completion_code,
                               SMTPc_ERR    *perr)
{
    CPU_CHAR    *reply;
    CPU_SIZE_T   len;

                                                                /* ----------------- TX CMD TO SERVER ----------------- */
    Str_Copy(SMTPc_Comm_Buf, SMTPc_CMD_RSET);
    Str_Cat(SMTPc_Comm_Buf, "\r\n");

    len = Str_Len(SMTPc_Comm_Buf);
    SMTPc_QueryServer(sock_id, SMTPc_Comm_Buf, len, perr);
    if (*perr != SMTPc_ERR_NONE) {
       *perr  = SMTPc_ERR_TX_FAILED;
        return ((CPU_CHAR *)0);
    }

                                                                /* ---------- RX SERVER'S RESPONSE & VALIDATE --------- */
    reply = SMTPc_RxReply(sock_id, perr);                       /* See Note #3.                                         */
    if (*perr != SMTPc_ERR_NONE) {
       *perr  = SMTPc_ERR_RX_FAILED;
        return ((CPU_CHAR *)0);
    }

    SMTPc_ParseReply(reply, completion_code, perr);
    switch (*perr) {
        case SMTPc_ERR_REP_POS:
             if (*completion_code == SMTPc_REP_250) {
                *perr = SMTPc_ERR_NONE;
             } else {
                *perr = SMTPc_ERR_REP;
             }
             break;

        case SMTPc_ERR_REP_TOO_SHORT:
            *perr = SMTPc_ERR_REP;
             reply = (CPU_CHAR *)0;
             break;

        default:
            *perr = SMTPc_ERR_REP;
             break;
    }

    return (reply);
}


/*
*********************************************************************************************************
*                                             SMTPc_QUIT()
*
* Description : (1) Build the QUIT command, send it to the server and validate reply.
*
*                   (a) Send command to the server
*                   (b) Receive server's reply and validate
*
*
* Argument(s) : sock_id         Socket ID.
*               completion_code Numeric value returned by server indicating command status.
*               perr            Pointer to variable that will hold the return error code from this
*                               function :
*
*                               SMTPc_ERR_NONE                      No error.
*                               SMTPc_ERR_TX_FAILED                 Error querying server.
*                               SMTPc_ERR_RX_FAILED                 Error receiving server reply.
*                               SMTPc_ERR_REP                       Error with reply.
*
* Return(s)   : Complete reply from the server, if NO reception error.
*
*               (CPU_CHAR *)0,                  otherwise.
*
* Caller(s)   : SMTPc_Disconnect().
*
* Note(s)     : (2) From RFC #2821, "the QUIT command specifies that the receiver MUST send an OK reply,
*                   and then close the transmission channel.  The receiver MUST NOT intentionally close
*                   the transmission channel until it receives and replies to a QUIT command (even if
*                   there was an error).  the sender MUST NOT intentionally close the transmission
*                   channel until it sends a QUIT command and SHOULD wait until it receives the reply
*                   (even if there was an error response to a previous command)".
*
*               (3) The server MUST send a 221 "Service closing transmission channel" reply to a QUIT
*                   command.
*********************************************************************************************************
*/

static  CPU_CHAR  *SMTPc_QUIT (NET_SOCK_ID   sock_id,
                               CPU_INT32U   *completion_code,
                               SMTPc_ERR    *perr)
{
    CPU_CHAR    *reply;
    CPU_SIZE_T   len;

                                                                /* ----------------- TX CMD TO SERVER ----------------- */
    Str_Copy(SMTPc_Comm_Buf, SMTPc_CMD_QUIT);
    Str_Cat(SMTPc_Comm_Buf, "\r\n");
    len = Str_Len(SMTPc_Comm_Buf);

    SMTPc_QueryServer(sock_id, SMTPc_Comm_Buf, len, perr);
    if (*perr != SMTPc_ERR_NONE) {
       *perr  = SMTPc_ERR_TX_FAILED;
        return ((CPU_CHAR *)0);
    }

                                                                /* ---------- RX SERVER'S RESPONSE & VALIDATE --------- */
    reply = SMTPc_RxReply(sock_id, perr);                       /* See Note #3.                                         */
    if (*perr != SMTPc_ERR_NONE) {
       *perr  = SMTPc_ERR_RX_FAILED;
        return ((CPU_CHAR *)0);
    }

    SMTPc_ParseReply(reply, completion_code, perr);
    switch (*perr) {
        case SMTPc_ERR_REP_POS:
             if (*completion_code == SMTPc_REP_221) {
                *perr = SMTPc_ERR_NONE;
             } else {
                *perr = SMTPc_ERR_REP;
             }
             break;

        case SMTPc_ERR_REP_TOO_SHORT:
            *perr = SMTPc_ERR_REP;
             reply = (CPU_CHAR *)0;
             break;

        default:
            *perr = SMTPc_ERR_REP;
             break;
    }

    return (reply);
}
