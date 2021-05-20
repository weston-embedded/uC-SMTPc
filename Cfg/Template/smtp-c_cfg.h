/*
*********************************************************************************************************
*                                              uC/SMTPc
*                               Simple Mail Transfer Protocol (client)
*
*                    Copyright 2004-2021 Silicon Laboratories Inc. www.silabs.com
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
*                                   SMTP CLIENT CONFIGURATION FILE
*
*                                              TEMPLATE
*
* Filename : smtp-c_cfg.h
* Version  : V2.01.01
*********************************************************************************************************
*/

#ifndef SMTPc_CFG_MODULE_PRESENT
#define SMTPc_CFG_MODULE_PRESENT


/*
*********************************************************************************************************
*                                  SMTPc ARGUMENT CHECK CONFIGURATION
*
* Note(s) : (1) Configure SMTPc_CFG_ARG_CHK_EXT_EN to enable/disable the SMTP client external argument
*               check feature :
*
*               (a) When ENABLED,  ALL arguments received from any port interface provided by the developer
*                   are checked/validated.
*
*               (b) When DISABLED, NO  arguments received from any port interface provided by the developer
*                   are checked/validated.
*********************************************************************************************************
*/
                                                                /* Configure external argument check feature ...        */
                                                                /* See Note 1.                                          */
#define  SMTPc_CFG_ARG_CHK_EXT_EN                   DEF_ENABLED
                                                                /* DEF_DISABLED     External argument check DISABLED    */
                                                                /* DEF_ENABLED      External argument check ENABLED     */
/*
*********************************************************************************************************
*                                                SMTPc
*
* Note(s) : (1) Default TCP port to use when calling SMTPc_Connect() without specifying a port
*               to connect to.  Standard listening port for SMTP servers is 25.
*
*           (2) Standard listening port for secure SMTP servers is 465.
*
*           (3) Configure SMTPc_CFG_AUTH_EN to enable/disable plaintext authentication.
*
*           (4) Configure maximum lengths for both username and password, when authentication is enabled.
*
*           (5) Corresponds to the maximum length of the displayed name associated with a mailbox,
*               including '\0'.  This length MUST be smaller than 600 in order to respect the same
*               limit.
*
*           (6) SMTPc_CFG_MSG_SUBJECT_LEN is the maximum length of the string containing the message subject,
*               including '\0'.  The length MUST be smaller than 900 characters in order to respect the
*               Internet Message Format line limit.
*
*           (7) Maximum length of the various arrays inside the SMTPc_MSG structure.
*********************************************************************************************************
*/

#define  SMTPc_CFG_IPPORT                                 25    /* Cfg SMTP        server IP port (see note #1).        */
#define  SMTPc_CFG_IPPORT_SECURE                         465    /* Cfg SMTP secure server IP port (see Note #2).        */

#define  SMTPc_CFG_MAX_CONN_REQ_TIMEOUT_MS              5000    /* Cfg max inactivity time (ms) on CONNECT.             */
#define  SMTPc_CFG_MAX_CONN_CLOSE_TIMEOUT_MS            5000    /* Cfg max inactivity time (ms) on DISCONNECT.          */

                                                                /* Cfg SMTP auth mechanism (see Note #3).               */
#define  SMTPc_CFG_AUTH_EN                      DEF_DISABLED
                                                                /*   DEF_DISABLED  PLAIN auth DISABLED                  */
                                                                /*   DEF_ENABLED   PLAIN auth ENABLED                   */

#define  SMTPc_CFG_USERNAME_MAX_LEN                       50    /* Cfg username max len (see Note #4).                  */
#define  SMTPc_CFG_PW_MAX_LEN                             10    /* Cfg pw       max len (see Note #4).                  */

#define  SMTPc_CFG_MBOX_NAME_DISP_LEN                     50    /* Cfg max len of sender's name   (see Note #5).        */
#define  SMTPc_CFG_MSG_SUBJECT_LEN                        50    /* Cfg max len of msg subject     (see Note #6).        */

                                                                /* See Note #7.                                         */
#define  SMTPc_CFG_MSG_MAX_TO                              5    /* Cfg msg max nbr of TO  recipients.                   */
#define  SMTPc_CFG_MSG_MAX_CC                              5    /* Cfg msg max nbr of CC  recipients.                   */
#define  SMTPc_CFG_MSG_MAX_BCC                             5    /* Cfg msg max nbr of BCC recipients.                   */
#define  SMTPc_CFG_MSG_MAX_ATTACH                          5    /* Cfg msg max nbr of msg attach.                       */

/*
*********************************************************************************************************
*                                                TRACING
*********************************************************************************************************
*/

#ifndef  TRACE_LEVEL_OFF
#define  TRACE_LEVEL_OFF                                   0
#endif

#ifndef  TRACE_LEVEL_INFO
#define  TRACE_LEVEL_INFO                                  1
#endif

#ifndef  TRACE_LEVEL_DBG
#define  TRACE_LEVEL_DBG                                   2
#endif

#define  SMTPc_TRACE_LEVEL                  TRACE_LEVEL_INFO
#define  SMTPc_TRACE                                  printf

#endif                                                          /* End of smtpc cfg module include.                     */
