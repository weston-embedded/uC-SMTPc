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
*                                      uC/SMTPc CMD SOURCE CODE
*
* Filename : smtp-c_cmd.c
* Version  : V2.01.00
*********************************************************************************************************
* Note(s)  : (1) Assumes the following versions (or more recent) of software modules are included in
*                the project build :
*
*                (a) uC/TCP-IP V3.03.00
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#define  MICRIUM_SOURCE
#define  SMTPc_CMD_MODULE

#include  "smtp-c_cmd.h"
#include  <Source/net_util.h>
#include  <Source/net_ascii.h>
#include  <Source/net_sock.h>


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define SMTPc_CMD_HELP_1                               "\r\nusage: smtp_send [options]\r\n\r\n"
#define SMTPc_CMD_HELP_2                               " -6,           Test SMTPc using IPv6 \r\n"
#define SMTPc_CMD_HELP_3                               " -4,           Test SMTPc using IPv4 \r\n"
#define SMTPc_CMD_HELP_4                               " -d,           Test SMTPc using server domain name (aspmx.l.google.com)\r\n"
#define SMTPc_CMD_HELP_5                               " -t,           Set the TO address used to send the mail\r\n"

#define SMTPc_CMD_OK                                   "OK"
#define SMTPc_CMD_FAIL                                 "FAIL "

#define SMTPc_CMD_SERVER_IPV4                          "192.168.0.2"
#define SMTPc_CMD_SERVER_IPV6                          "fe80::1234:5678:"
#define SMTPc_CMD_SERVER_DOMAIN_NAME                   "aspmx.l.google.com"

#define SMTPc_CMD_MAILBOX_FROM_NAME                    "Test Name From"
#define SMTPc_CMD_MAILBOX_FROM_ADDR                    "webmaster@mail.smtptest.com"
#define SMTPc_CMD_MAILBOX_TO_NAME                      "Test Name To"
#define SMTPc_CMD_MAILBOX_TO_ADDR                      "testto@mail.smtptest.com"


#define SMTPc_CMD_USERNAME                             "webmaster@mail.smtptest.com"
#define SMTPc_CMD_PW                                   "password"

#define SMTPc_CMD_MSG_SUBJECT                          "Test"
#define SMTPc_CMD_MSG                                  "This is a test message"
#define SMTPc_CMD_MAILBOX_CC_ADDR                      "testcc1@mail.smtptest.com"
#define SMTPc_CMD_MAILBOX_CC_NAME                      "Test Name CC1"
#define SMTPc_CMD_MAILBOX_BCC_ADDR                     "testbcc1@mail.smtptest.com"
#define SMTPc_CMD_MAILBOX_BCC_NAME                     "Test Name BCC 1"

#define SMTPc_CMD_PARSER_DNS                           ASCII_CHAR_LATIN_LOWER_D
#define SMTPc_CMD_PARSER_IPv6                          ASCII_CHAR_DIGIT_SIX
#define SMTPc_CMD_PARSER_IPv4                          ASCII_CHAR_DIGIT_FOUR
#define SMTPc_CMD_ARG_PARSER_CMD_BEGIN                 ASCII_CHAR_HYPHEN_MINUS
#define SMTPc_CMD_PARSER_TO                            ASCII_CHAR_LATIN_LOWER_T


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

CPU_INT16S  SMTPcCmd_Send            (CPU_INT16U          argc,
                                      CPU_CHAR           *p_argv[],
                                      SHELL_OUT_FNCT      out_fnct,
                                      SHELL_CMD_PARAM    *p_cmd_param);

CPU_INT16S  SMTPcCmd_Help            (CPU_INT16U          argc,
                                      CPU_CHAR           *p_argv[],
                                      SHELL_OUT_FNCT      out_fnct,
                                      SHELL_CMD_PARAM    *p_cmd_param);


/*
*********************************************************************************************************
*                                            LOCAL TABLES
*********************************************************************************************************
*/

static  SHELL_CMD SMTPc_CmdTbl[] =
{
    {"smtp_send" , SMTPcCmd_Send},
    {"smtp_help" , SMTPcCmd_Help},
    {0, 0}
};


/*
*********************************************************************************************************
*                                           SMTPcCmd_Init()
*
* Description : Add uC/SMTPc cmd stubs to uC-Shell.
*
* Argument(s) : p_err    is a pointer to an error code which will be returned to your application:
*
*                             SMTPc_CMD_ERR_NONE           No error.
*
*                             SMTPc_CMD_ERR_SHELL_INIT     Command table not added to uC-Shell
*
* Return(s)   : none.
*
* Caller(s)   : AppTaskStart().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  SMTPcCmd_Init (SMTPc_CMD_ERR  *p_err)
{
    SHELL_ERR  err;


    Shell_CmdTblAdd("smtp", SMTPc_CmdTbl, &err);

    if (err == SHELL_ERR_NONE) {
        *p_err = SMTPc_CMD_ERR_NONE;
    } else {
        *p_err = SMTPc_CMD_ERR_SHELL_INIT;
    }
}


/*
*********************************************************************************************************
*                                           SMTPc_Cmd_Send()
*
* Description : Send a predefine test e-mail.
*
* Argument(s) : argc            is a count of the arguments supplied.
*
*               p_argv          an array of pointers to the strings which are those arguments.
*
*               out_fnct        is a callback to a respond to the requester.
*
*               p_cmd_param     is a pointer to additional information to pass to the command.
*
*
* Return(s)   : The number of positive data octets transmitted, if NO errors
*
*               SHELL_OUT_ERR,                                  otherwise
*
* Caller(s)   : AppTaskStart().
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_INT16S SMTPcCmd_Send (CPU_INT16U        argc,
                          CPU_CHAR         *p_argv[],
                          SHELL_OUT_FNCT    out_fnct,
                          SHELL_CMD_PARAM  *p_cmd_param)
{
    SMTPc_ERR        err;
    CPU_INT16S       output;
    CPU_INT16S       ret_val;
    CPU_CHAR        *p_server_addr;
    CPU_CHAR        *p_to_addr;
    CPU_INT16U       error_code;
    CPU_CHAR         reply_buf[16];
    CPU_INT16U       cmd_namd_len;
    CPU_INT08U       i;
    SMTPc_MBOX       from_mbox;
    SMTPc_MBOX       to_mbox;
    SMTPc_MSG        mail;


    p_to_addr     = SMTPc_CMD_MAILBOX_TO_ADDR;
    p_server_addr = SMTPc_CMD_SERVER_IPV4;
                                                                /* Parse Arguments.                                     */
    if (argc != 0) {
        for (i = 1; i < argc; i++) {
            if (*p_argv[i] == SMTPc_CMD_ARG_PARSER_CMD_BEGIN) {
                switch (*(p_argv[i] + 1)) {
                    case SMTPc_CMD_PARSER_IPv6:
                         p_server_addr = SMTPc_CMD_SERVER_IPV6;

                         if (argc != i + 1) {
                             if (*p_argv[i+1] != SMTPc_CMD_ARG_PARSER_CMD_BEGIN) {
                                 p_server_addr = p_argv[i+1];
                                 i++;
                             }
                         }
                         break;

                    case SMTPc_CMD_PARSER_IPv4:
                         p_server_addr = SMTPc_CMD_SERVER_IPV4;

                         if (argc != i + 1) {
                             if (*p_argv[i+1] != SMTPc_CMD_ARG_PARSER_CMD_BEGIN) {
                                 p_server_addr = p_argv[i+1];
                                 i++;
                             }
                         }
                         break;

                    case SMTPc_CMD_PARSER_DNS:
                         p_server_addr = SMTPc_CMD_SERVER_DOMAIN_NAME;

                         if (argc != i + 1) {
                             if (*p_argv[i+1] != SMTPc_CMD_ARG_PARSER_CMD_BEGIN) {
                                 p_server_addr = p_argv[i+1];
                                 i++;
                             }
                         }
                         break;

                    case SMTPc_CMD_PARSER_TO:
                          if (argc != i + 1) {
                              if (*p_argv[i+1] != SMTPc_CMD_ARG_PARSER_CMD_BEGIN) {
                                  p_to_addr = p_argv[i+1];
                                  i++;
                              } else {
                                  err =  SMTPc_ERR_INVALID_ADDR;
                                  goto exit_fail;
                              }
                          } else {
                              err =  SMTPc_ERR_INVALID_ADDR;
                              goto exit_fail;
                          }
                          break;

                    default:
                         err =  SMTPc_ERR_INVALID_ADDR;
                         goto exit_fail;
                         break;

                }
            }
        }
    } else {
        err = SMTPc_ERR_INVALID_ADDR;
        goto exit_fail;
    }

                                                                /* Initialize the message obj.                          */
    SMTPc_SetMsg (&mail, &err);
    if (err != SMTPc_ERR_NONE) {
        goto exit_fail;
    }
                                                                /* Set Mail Envelop.                                    */
    mail.From = &from_mbox;
    SMTPc_SetMbox(mail.From, SMTPc_CMD_MAILBOX_FROM_NAME, SMTPc_CMD_MAILBOX_FROM_ADDR, &err);
    if (err != SMTPc_ERR_NONE) {
        goto exit_fail;
    }
    mail.ToArray[0] = &to_mbox;
    SMTPc_SetMbox(mail.ToArray[0], SMTPc_CMD_MAILBOX_TO_NAME, p_to_addr, &err);
    if (err != SMTPc_ERR_NONE) {
        goto exit_fail;
    }

                                                                /* Set Mail Content.                                    */
    mail.Subject           = SMTPc_CMD_MSG_SUBJECT;
    mail.ContentBodyMsg    = SMTPc_CMD_MSG;
    mail.ContentBodyMsgLen = sizeof(SMTPc_CMD_MSG);

                                                                /* Send Mail.                                           */
    SMTPc_SendMail( p_server_addr,
                    0,
                    SMTPc_CMD_USERNAME,
                    SMTPc_CMD_PW,
                    DEF_NULL,
                   &mail,
                   &err);
    if (err == SMTPc_ERR_NONE) {
        Str_Copy(&reply_buf[0], SMTPc_CMD_OK);
        cmd_namd_len = Str_Len(reply_buf);
    } else {

exit_fail:
        error_code = err;
        Str_Copy(&reply_buf[0], SMTPc_CMD_FAIL);
        cmd_namd_len = Str_Len(reply_buf);

        Str_FmtNbr_Int32U(error_code, 5, 10, ' ', DEF_YES, DEF_YES, &reply_buf[cmd_namd_len]);
        cmd_namd_len = Str_Len(reply_buf);
    }

    reply_buf[cmd_namd_len + 0] = '\r';
    reply_buf[cmd_namd_len + 1] = '\n';
    reply_buf[cmd_namd_len + 2] = '\0';

    cmd_namd_len = Str_Len(reply_buf);

    output = out_fnct(&reply_buf[0],
            cmd_namd_len,
            p_cmd_param->pout_opt);

    switch (output) {
        case SHELL_OUT_RTN_CODE_CONN_CLOSED:
        case SHELL_OUT_ERR:
             ret_val = SHELL_EXEC_ERR;
             break;
        default:
             ret_val = output;
    }

    return (ret_val);
}


/*
*********************************************************************************************************
*                                           SMTPc_Cmd_Help()
*
* Description : Print SMTPc command help.
*
* Argument(s) : argc            is a count of the arguments supplied.
*
*               p_argv          an array of pointers to the strings which are those arguments.
*
*               out_fnct        is a callback to a respond to the requester.
*
*               p_cmd_param     is a pointer to additional information to pass to the command.
*
*
* Return(s)   : The number of positive data octets transmitted, if NO errors
*
*               SHELL_OUT_ERR,                                  otherwise
*
* Caller(s)   : AppTaskStart().
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_INT16S SMTPcCmd_Help (CPU_INT16U        argc,
                          CPU_CHAR         *p_argv[],
                          SHELL_OUT_FNCT    out_fnct,
                          SHELL_CMD_PARAM  *p_cmd_param)
{
    CPU_INT16U cmd_namd_len;
    CPU_INT16S output;
    CPU_INT16S ret_val;


    cmd_namd_len = Str_Len(SMTPc_CMD_HELP_1);
    output       = out_fnct(SMTPc_CMD_HELP_1,
                            cmd_namd_len,
                            p_cmd_param->pout_opt);

    cmd_namd_len = Str_Len(SMTPc_CMD_HELP_2);
    output       = out_fnct(SMTPc_CMD_HELP_2,
                            cmd_namd_len,
                            p_cmd_param->pout_opt);

    cmd_namd_len = Str_Len(SMTPc_CMD_HELP_3);
    output       = out_fnct(SMTPc_CMD_HELP_3,
                            cmd_namd_len,
                            p_cmd_param->pout_opt);

    cmd_namd_len = Str_Len(SMTPc_CMD_HELP_4);
    output       = out_fnct(SMTPc_CMD_HELP_4,
                            cmd_namd_len,
                            p_cmd_param->pout_opt);

    cmd_namd_len = Str_Len(SMTPc_CMD_HELP_5);
    output       = out_fnct(SMTPc_CMD_HELP_5,
                            cmd_namd_len,
                            p_cmd_param->pout_opt);

    switch (output) {
        case SHELL_OUT_RTN_CODE_CONN_CLOSED:
        case SHELL_OUT_ERR:
             ret_val = SHELL_EXEC_ERR;
             break;
        default:
             ret_val = output;
    }

    return (ret_val);
}

