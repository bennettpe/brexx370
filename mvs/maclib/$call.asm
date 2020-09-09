         MACRO
&NAME    $CALL &EP=,&EPLOC=,&PARAM=,&PARMLOC=
         MNOTE *,'       $CALL     VERSION 001 06/06/75  06/06/75  GPW'
.**********************************************************************
.*                                                                    *
.* $CALL                                                              *
.*                                                                    *
.* FUNCTION       GENERATE CODE REQUIRED TO PASS CONTROL TO ANOTHER   *
.*                CSECT AND PASS PARAMETERS IF REQUIRED.              *
.*                                                                    *
.* DESCRIPTION    STANDARD IBM LINKAGE CODE IS GENERATED BY THE MACRO *
.*                TO PASS CONTROL TO A SPECIFIED LOAD MODULE.  A LIST *
.*                OF PARAMETERS MAY ALSO BE CONSTRUCTED.              *
.*                                                                    *
.*                A FULLWORD NO-OP CONTAINING AN IDENTIFICATION       *
.*                NUMBER WILL BE PLACED AFTER THE BALR USED TO        *
.*                TRANSFER CONTROL.  THIS VALUE WILL BE DISPLAYED IN  *
.*                DUMPS.  THE STARTING VALUE IS 10 AND IS INCREMENTED *
.*                BY 10 FOR EACH CALL.  THE VALUE IS DEFINED BY THE   *
.*                GLOBAL SYMBOL $CALLID.                              *
.*                                                                    *
.* SYNTAX         NAME     $CALL     EP=SYM1                          *
.*                                   EPLOC=SYM2                       *
.*                                                                    *
.*                                   PARAM=(SYM-LIST)                 *
.*                                   PARMLOC=SYM3                     *
.*                                                                    *
.*                                                                    *
.*                NAME   - A SYMBOLIC TAG ASSIGNED TO THE FIRST       *
.*                         INSTRUCTION GENERATED.                     *
.*                                                                    *
.*                EP     - THE NAME OF THE CSECT TO WHICH CONTROL IS  *
.*                         TO BE TRANSFERRED.  A V TYPE ADDRESS       *
.*                         CONSTANT WILL BE GENERATED.                *
.*                                                                    *
.*                EPLOC  - THE NAME OF A FULLWORD CONTAINING THE      *
.*                         ADDRESS OF THE CSECT TO WHICH CONTROL IS   *
.*                         TO BE TRANSFERRED.                         *
.*                                                                    *
.*                         NOTE - IF EP IS SPECIFIED, EPLOC IS        *
.*                                IGNORED.                            *
.*                                                                    *
.*                PARAM  - A LIST OF SYMBOLIC NAMES OF PARAMETERS TO  *
.*                         BE PASSED TO THE CALLED ROUTINE.  AN       *
.*                         IN-LINE PARAMETER LIST WILL BE GENERATED.  *
.*                         THE ADDRESS OF THE LIST WILL BE PLACED IN  *
.*                         REGISTER 1.  THE HIGH ORDER BIT OF THE     *
.*                         LAST WORD IN THE LIST WILL BE SET TO 1.    *
.*                                                                    *
.*                PARMLOC- THE NAME OF THE FIRST WORD OF A USER       *
.*                         SUPPLIED PARAMETER LIST.                   *
.*                                                                    *
.*                         NOTE - IF PARAM IS SPECIFIED, PARMLOC IS   *
.*                                IGNORED.                            *
.*                                                                    *
.* ERRORS         EP OR EPLOC MUST BE SPECIFIED.  IF NEITHER VALUE    *
.*                IS PROVIDED, AN ERROR MESSAGE WILL BE GENERATED.    *
.*                THE ERROR IS SEVERITY CODE 8.                       *
.*                                                                    *
.* EXAMPLE        EX1      $CALL EP=GDATE,PARAM=DATE                  *
.*                              .                                     *
.*                              .                                     *
.*                              .                                     *
.*                DATE     DC    CL12' '                              *
.*                                                                    *
.*                                                                    *
.*                EX2      $CALL EPLOC=SUBRADR,PARMLOC=PARMLIST       *
.*                              .                                     *
.*                              .                                     *
.*                              .                                     *
.*                SUBRADR  DC    V(GDATE)                             *
.*                PARMLIST DC    A(DATE)                              *
.*                              .                                     *
.*                              .                                     *
.*                              .                                     *
.*                DATE     DC    CL12' '                              *
.*                                                                    *
.* GLOBAL SYMBOLS                                                     *
.*                                                                    *
.*                NAME     TYPE  USE                                  *
.*                                                                    *
.*                &$CALLID   A   USED TO GENERATE AN IDENTIFICATION   *
.*                               NUMBER IN A NO-OP FOLLOWING THE BALR *
.*                               USED TO TRANSFER CONTROL.  THE VALUE *
.*                               IS INCREMENTED BY 10 FOLLOWING EACH  *
.*                               CALL.  THIS VALUE WILL BE DISPLAYED  *
.*                               IN THE TRACEBACK PROVIDED WITH       *
.*                               DUMPS.                               *
.* MACROS CALLED                                                      *
.*                                                                    *
.*                NONE                                                *
.*                                                                    *
.**********************************************************************
.*
         GBLA  &$CALLID
.*
         LCLA  &COUNT
         LCLC  &ID
.*
&ID      SETC  '&NAME'
.*-------DO I SET UP PARM LIST?
         AIF   ('&PARAM' EQ '').PARMADR
         AIF   ('&PARAM' EQ '(R1)').PARMADR
         CNOP  0,4                      ALIGN TO FULL WORD
&COUNT   SETA  4*N'&PARAM+4
&ID      BAL   R1,*+&COUNT              BRANCH AROUND LIST
&ID      SETC  ''
&COUNT   SETA  1
.LOOP    ANOP
         AIF   (&COUNT EQ N'&PARAM).LAST
         DC    A(&PARAM(&COUNT))        PARAMETER &COUNT
&COUNT   SETA  &COUNT+1
         AGO   .LOOP
.LAST    DC    X'80',AL3(&PARAM(&COUNT)) LAST PARAMETER
         AGO   .ENTRY
.*-------SET UP PARM ADDR
.PARMADR AIF   ('&PARMLOC' EQ '').ENTRY
&ID      LA    R1,&PARMLOC              LOAD PARM LIST ADDRESS
&ID      SETC  ''
.*-------SET UP ENTRY POINT
.ENTRY   AIF   ('&EP' EQ '').EPLOC
&ID      L     R15,=V(&EP)              LOAD ENTRY ADDRESS    120475SH
         AGO   .BALR
.*-------SET UP ENTRY POINT
.EPLOC   AIF   ('&EPLOC' EQ '').BAD
&ID      L     R15,&EPLOC               LOAD ENTRY POINT ADDRESS
.*-------BRANCH TO SUBROUTINE
.BALR    BALR  R14,R15                  BRANCH TO SUBROUTINE
&$CALLID SETA  &$CALLID+10
         DC    X'4700',AL2(&$CALLID)    NO-OP, ID
         MEXIT
.*-------NO ENTRY POINT
.BAD     MNOTE 8,'*** ERROR - NO ENTRY POINT SPECIFIED'
         MEND