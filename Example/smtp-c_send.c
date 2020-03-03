/*
*********************************************************************************************************
*                                            EXAMPLE CODE
*
*               This file is provided as an example on how to use Micrium products.
*
*               Please feel free to use any application code labeled as 'EXAMPLE CODE' in
*               your application products.  Example code may be used as is, in whole or in
*               part, or may be used as a reference only. This file can be modified as
*               required to meet the end-product requirements.
*
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                               EXAMPLE
*
*                                             SMTP CLIENT

*
* Filename : smtp-c_send.c
* Version  : V2.01.00
*********************************************************************************************************
* Note(s)  : (1) This example show how to send an e-mail with SMTP client
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <Source/smtp-c.h>


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define SERVER_IPV4          "192.168.0.5"
#define SERVER_IPV6          "fe80::1234:5678"
#define SERVER_DOMAIN_NAME   "aspmx.l.google.com"

#define MAILBOX_FROM_NAME    "John Doe"
#define MAILBOX_FROM_ADDR    "john.doe@foo.bar"
#define USERNAME             "john.doe@foo.bar"
#define PASSWORD             "123"

#define MAILBOX_TO_NAME_1    "Jane Doe"
#define MAILBOX_TO_ADDR_1    "jane.doe@foo.bar"

#define MAILBOX_TO_NAME_2    "Jonnie Doe"
#define MAILBOX_TO_ADDR_2    "jonnie.doe@foo.bar"

#define MAILBOX_CC_NAME      "Janie Doe"
#define MAILBOX_CC_ADDR      "janie.doe@foo.bar"

#define MAILBOX_BCC_NAME     "Richard Roe"
#define MAILBOX_BCC_ADDR     "richard.roe@foo.bar"

#define MSG_SUBJECT          "Test"
#define MSG                  "This is a test message"


/*
*********************************************************************************************************
*                                          AppMailSend()
*
* Description : Process all the step to prepare and send an email.
*
* Argument(s) : none.
*
* Return(s)   : DEF_OK,   No error, message sent.
*
*               DEF_FAIL, Otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_BOOLEAN AppMailSend (void)
{
    CPU_CHAR                *p_server_addr;
    SMTPc_MBOX               from_mbox;
    SMTPc_MBOX               to_mbox1;
    SMTPc_MBOX               to_mbox2;
    SMTPc_MBOX               cc_mbox;
    SMTPc_MBOX               bcc_mbox;
    SMTPc_MSG                mail;
    SMTPc_ERR                err;
    CPU_INT16U               port;
    NET_APP_SOCK_SECURE_CFG *p_secure_cfg;


    p_server_addr = SERVER_IPV4;
                                                                /* ------------- INITIALIZE THE MAIL OBJ -------------- */
                                                                /* Set the email object mailbox and content to Null.    */
                                                                /* All fields set to Null will be ignore in the email...*/
                                                                /* ...transmission.                                     */
    SMTPc_SetMsg (&mail, &err);
    if (err != SMTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
                                                                /* ---------------- SET MAIL ENVELOP ------------------ */
                                                                /* Set all the mailbox which the email is related to.   */
                                                                /* SMTPc_MSG need at least the TO and FROM mailbox...   */
                                                                /* ...in order to be sent properly.                     */

    mail.From = &from_mbox;                                     /* Set the FROM mailbox of the e-mail.                  */
    SMTPc_SetMbox(mail.From, MAILBOX_FROM_NAME, MAILBOX_FROM_ADDR, &err);
    if (err != SMTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    mail.ToArray[0] = &to_mbox1;                                /* Set the TO mailbox of the e-mail.                    */
    SMTPc_SetMbox(mail.ToArray[0], MAILBOX_TO_NAME_1, MAILBOX_TO_ADDR_1, &err);
    if (err != SMTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
                                                                /* Set the CC mailbox of the e-mail.                    */
    mail.CCArray[0] = &cc_mbox;
    SMTPc_SetMbox(mail.CCArray[0], MAILBOX_CC_NAME, MAILBOX_CC_ADDR, &err);
    if (err != SMTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
                                                                /* Set the BCC mailbox of the e-mail.                   */
    mail.BCCArray[0] = &bcc_mbox;
    SMTPc_SetMbox(mail.BCCArray[0], MAILBOX_BCC_NAME, MAILBOX_BCC_ADDR, &err);
    if (err != SMTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
                                                                /* The TO,CC and BCC mailbox array allow to send...     */
                                                                /* ... e-mails to multiple destination.                 */
    mail.ToArray[1] = &to_mbox2;
    SMTPc_SetMbox(mail.ToArray[1], MAILBOX_TO_NAME_2, MAILBOX_TO_ADDR_2, &err);
    if (err != SMTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* ----------------- SET MAIL CONTENT ----------------- */
                                                                /* Set the message subject and body.                    */
    mail.Subject           = MSG_SUBJECT;
    mail.ContentBodyMsg    = MSG;
    mail.ContentBodyMsgLen = sizeof(MSG);

                                                                /* ---------------- SET CONN SECURITY ----------------- */
    port                   = SMTPc_CFG_IPPORT;
    p_secure_cfg           = DEF_NULL;

                                                                /* -------------------- SEND MAIL --------------------- */
    SMTPc_SendMail( p_server_addr,
                    port,
                    USERNAME,
                    PASSWORD,
                    p_secure_cfg,
                   &mail,
                   &err);
    if (err != SMTPc_ERR_NONE) {
        return (DEF_FAIL);
    }
    return (DEF_OK);
}

