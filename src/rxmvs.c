#include <stdlib.h>
#include <stdio.h>
#include "irx.h"
#include "rexx.h"
#include "rxdefs.h"
#include "rxmvsext.h"
#include "util.h"
#include "rxtcp.h"
#include "rxrac.h"
#include "dynit.h"
#ifdef __DEBUG__
#include "bmem.h"
#endif

RX_ENVIRONMENT_CTX_PTR environment = NULL;

#ifdef JCC
extern FILE * stdin;
extern FILE * stdout;
extern FILE * stderr;
#endif

/* FLAG2 */
const unsigned char _TSOFG  = 0x1; // hex for 0000 0001
const unsigned char _TSOBG  = 0x2; // hex for 0000 0010
const unsigned char _EXEC   = 0x4; // hex for 0000 0100
const unsigned char _ISPF   = 0x8; // hex for 0000 1000
/* FLAG3 */
const unsigned char _STDIN  = 0x1; // hex for 0000 0001
const unsigned char _STDOUT = 0x2; // hex for 0000 0010
const unsigned char _STDERR = 0x4; // hex for 0000 0100

#ifdef __CROSS__
# include "jccdummy.h"
#else
extern char* _style;
extern void ** entry_R13;
extern int __libc_tso_status;
#endif

/* ------------------------------------------------------------------------------------------------------------------ */
#define BYTE unsigned char
typedef struct {
    BYTE vlvl;     // version level
    BYTE mlvl;     // modification level
    BYTE res1;     // reserver
    BYTE chgss;    // X   { SS }
    BYTE credtÝ4¨; // PL4 { signed packed
    BYTE chgdtÝ4¨; // PL4   julian date   }
    BYTE chgtmÝ2¨; // XL2 { HHMM }
    short curr;    // number of current  records
    short init;    // number of initial  records
    short mod ;    // number of modified records
    char uidÝ10¨;
} USER_DATA, *P_USER_DATA;

void julian2gregorian(int year, int day, char **date)
{
    static const int month_lenÝ¨ = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    int leap = (year % 4 == 0) && (year % 100 != 0 || year % 400 == 0);
    int day_of_month = day;
    int month;

    for (month = 0; month < 12; month ++) {

        int mlen = month_lenÝmonth¨;

        if (leap && month == 1)
            mlen ++;

        if (mlen >= day_of_month)
            break;

        day_of_month -= mlen;

    }

    sprintf(*date, "%.2d-%.2d-%.2d", year, month+1, day_of_month);
}

int getYear(BYTE flag, BYTE yy) {
    int year = 0;

    char tmpÝ2¨;
    bzero(tmp, 2);

    sprintf(tmp, "%x", yy);
    sscanf (tmp, "%d", &year);

    /*
    if (flag == 0x01) {
        year = year + 2000;
    } else {
        year = year + 1900;
    }
    */

    return year;
}

int getDay(BYTE byte1, BYTE byte2) {
    int day = 0;

    char tmpÝ3¨;
    bzero(tmp, 3);

    sprintf(tmp, "%.1x%.1x%.1x", (byte1 >> 4) & 0x0F, byte1 & 0x0F, (byte2 >> 4) & 0x0F );
    sscanf(tmp, "%d", &day);

    return day;
}
/* ------------------------------------------------------------------------------------------------------------------ */

static int i;

/* internal function prototypes */
int GetClistVar(PLstr name, PLstr value);
int SetClistVar(PLstr name, PLstr value);

void parseArgs(char **array, char *str);
void parseDCB(FILE *pFile);
int checkNameLength(long lName);
int checkValueLength(long lValue);
int checkVariableBlacklist(PLstr name);
int reopen(int fp);

void Lcryptall(PLstr to, PLstr from, PLstr pw, int rounds,int mode);
int _EncryptString(const PLstr to, const PLstr from, const PLstr password);
void _rotate(PLstr to,PLstr from, int start,int slen);
void Lhash(const PLstr to, const PLstr from, long slots) ;


#ifdef __CROSS__
int __get_ddndsnmemb (int handle, char * ddn, char * dsn,
                      char * member, char * serial, unsigned char * flags);

#endif

#define BLACKLIST_SIZE 8
char *RX_VAR_BLACKLISTÝBLACKLIST_SIZE¨ = {"RC", "LASTCC", "SIGL", "RESULT", "SYSPREF", "SYSUID", "SYSENV", "SYSISPF"};

#ifdef __CROSS__
/* ------------------------------------------------------------------------------------*/
char*
getNextVar(void** nextPtr)
{
    BinTree *currentTree = NULL;
    BinLeaf *leaf  = NULL;
    PLstr    value = NULL;

    currentTree = &(_procÝ_rx_proc¨.scopeÝ0¨);

    if (*nextPtr == 0) {
        leaf = BinMin(currentTree->parent);
    }
    else {
        leaf = BinSuccessor(leaf);
    }

    return ((PLstr)leaf->value)->pstr;


    /*
    while (leaf == NULL && i < VARTREES) {
        if (nextPtr == NULL) {
            leaf = BinMin(_procÝ_rx_proc¨.scopeÝi¨.parent);
        } else {
            leaf = BinSuccessor(nextPtr);
        }

        if (leaf != NULL) {
            value = (PLstr) leaf->value;
            leaf = BinSuccessor(leaf);
            if (leaf != NULL) {
                nextPtr = BinSuccessor(leaf);
            } else {
                nextPtr = 0;
            }
        } else {
            i++;
        }
    }

    return LSTR(*value);
    */
}
/* ------------------------------------------------------------------------------------*/
#endif

void R_catchIt(int func)
{
    int rc = -1;

    RX_TSO_PARAMS_PTR  tso_parameter;

    void ** cppl;

    if (__libc_tso_status == 1 && entry_R13 Ý6¨ != 0) {

        rc = 42;

        cppl = entry_R13Ý6¨;

        tso_parameter = malloc(sizeof(RX_TSO_PARAMS));
        memset(tso_parameter, 00, sizeof(RX_TSO_PARAMS));

        tso_parameter->cppladdr = (unsigned int *) cppl;

        strcpy(tso_parameter->ddin, "STDIN   ");
        strcpy(tso_parameter->ddout, "STDOUT  ");

        rc = call_rxtso(tso_parameter);
    }

    Licpy(ARGR, rc);

}

void R_dumpIt(int func)
{
    void *ptr  = 0;
    int   size = 0;
    long  adr  = 0;

    if (ARGN > 2 || ARGN < 1) {
        Lerror(ERR_INCORRECT_CALL,0);
    }

    if (ARGN == 1) {

    } else {
        Lx2d(ARGR,ARG1,0);    /* using ARGR as temp field for conversion */
        adr = Lrdint(ARGR);
        if (adr < 0) {
            Lerror(ERR_INCORRECT_CALL, 0);
        }

        ptr = (void *)adr;
        size = Lrdint(ARG2);
    }



    DumpHex((unsigned char *)ptr, size);
}

void R_wto(int func)
{
    RX_WTO_PARAMS_PTR params;

    char  *msgptr = NULL;
    size_t msglen = 0;
    int      cc     = 0;
    void     *wk;

    if (ARGN != 1)
        Lerror(ERR_INCORRECT_CALL,0);

    LASCIIZ(*ARG1);
    get_s(1);

    msglen = MIN(strlen((char *)LSTR(*ARG1)),80);

    if (msglen > 0) {
        msgptr = malloc(msglen);
        params = malloc(sizeof(RX_WTO_PARAMS));
        wk     = malloc(256);

        memset(msgptr,0,80);
        memcpy(msgptr,(char *)LSTR(*ARG1),msglen);

        params->msgadr       = msgptr;
        params->msgladr      = (unsigned int *)&msglen;
        params->ccadr        = (unsigned *)&cc;
        params->wkadr        = (unsigned *)wk;

        call_rxwto(params);

        free(wk);
        free(params);
        free(msgptr);
    }
}

void R_listIt(int func)
{
    BinTree tree;
    int	j;
    if (ARGN > 1 ) {
        Lstr lsFuncName,lsMaxArg;

        LINITSTR(lsFuncName)
        LINITSTR(lsMaxArg)

        Lfx(&lsFuncName,6);
        Lfx(&lsMaxArg, 4);

        Lscpy(&lsFuncName, "ListIT");
        Licpy(&lsMaxArg,1);

        Lerror(ERR_INCORRECT_CALL,4,&lsFuncName, &lsMaxArg);
    }

    if (ARG1 != NULL && ARG1->pstr == NULL) {
        printf("LISTIT: invalid parameters, maybe enclose in quotes\n");
        Lerror(ERR_INCORRECT_CALL,4,1);
    }

    tree = _procÝ_rx_proc¨.scopeÝ0¨;

    if (ARG1 == NULL || LSTR(*ARG1)Ý0¨ == 0) {
        printf("List all Variables\n");
        printf("------------------\n");
        BinPrint(tree.parent, NULL);
    } else {
        LASCIIZ(*ARG1) ;
        Lupper(ARG1);
        printf("List Variables with Prefix '%s'\n",ARG1->pstr);
        printf("%.*s\n", 29+ARG1->len,
               "-------------------------------------------------------");
        BinPrint(tree.parent, ARG1);
    }
}
void R_vlist(int func)
{
    BinTree tree;
    int	j;
    int mode=1;
    if (ARGN > 2 ) {
        Lstr lsFuncName,lsMaxArg;

        LINITSTR(lsFuncName)
        LINITSTR(lsMaxArg)

        Lfx(&lsFuncName,5);
        Lfx(&lsMaxArg, 4);

        Lscpy(&lsFuncName, "VList");
        Licpy(&lsMaxArg,2);

        Lerror(ERR_INCORRECT_CALL,4,&lsFuncName, &lsMaxArg);
    }

    if (ARG1 != NULL && ARG1->pstr == NULL) {
        printf("VLIST: invalid parameters, maybe enclose in quotes\n");
        Lerror(ERR_INCORRECT_CALL,4,1);
    }
    if (exist(2)) {
        L2STR(ARG2);
        Lupper(ARG2);
        if (LSTR(*ARG2)Ý0¨ == 'V') mode = 1;
        else if (LSTR(*ARG2)Ý0¨ == 'N') mode = 2;
    }

    tree = _procÝ_rx_proc¨.scopeÝ0¨;

    if (ARG1 == NULL || LSTR(*ARG1)Ý0¨ == 0) {
       BinVarDump(ARGR,tree.parent, NULL,mode);
    } else {
       LASCIIZ(*ARG1) ;
       Lupper(ARG1);
       BinVarDump(ARGR, tree.parent, ARG1,mode);
    }
}
void R_bldl(int func) {
    int found=0;
    if (ARGN != 1 || LLEN(*ARG1)==0) Lerror(ERR_INCORRECT_CALL,0);
    LASCIIZ(*ARG1) ;
    Lupper(ARG1);

    if (findLoadModule((char *)LSTR(*ARG1))) found=1;
    Licpy(ARGR,found);
}
void R_upper(int func) {
    if (ARGN != 1) Lerror(ERR_INCORRECT_CALL,0);

    if (LTYPE(*ARG1) != LSTRING_TY) {
        L2str(ARG1);
    };
    LASCIIZ(*ARG1) ;
    Lupper(ARG1);
    Lstrcpy(ARGR,ARG1);
}
void R_lower(int func) {
    if (ARGN != 1) Lerror(ERR_INCORRECT_CALL,0);

    if (LTYPE(*ARG1) != LSTRING_TY) {
        L2str(ARG1);
    };
    LASCIIZ(*ARG1) ;
    Llower(ARG1);
    Lstrcpy(ARGR,ARG1);
}

void R_lastword(int func) {
    long	i=0, lwi=0, lwe=0;
    if (ARGN != 1) Lerror(ERR_INCORRECT_CALL,0);
    if (LLEN(*ARG1)==0) {
        LZEROSTR(*ARGR);
        return;
    }
    L2STR(ARG1);
    LASCIIZ(*ARG1) ;

    for (;;) {
        LSKIPBLANKS(*ARG1,i);
        if (i>=LLEN(*ARG1)) break;
        lwi=i;
        LSKIPWORD(*ARG1,i);
        lwe=i;
    }
    if (lwi>0)  _Lsubstr(ARGR,ARG1,lwi+1,lwe-lwi);
    else LZEROSTR(*ARGR);
}
void R_join(int func) {
    int mlen = 0, slen=0, i = 0,j=0;
    Lstr joins, tabin;
    if (ARGN >3 || ARGN<2 || ARG1==NULL || ARG2==NULL) Lerror(ERR_INCORRECT_CALL, 0);
    if (LLEN(*ARG1) <1) {
       Lstrcpy(ARGR, ARG2);
       return;
    }
    if (LLEN(*ARG2) <1) {
        Lstrcpy(ARGR, ARG1);
        return;
    }
    if (LLEN(*ARG1) > LLEN(*ARG2)) mlen = LLEN(*ARG1);
    else mlen = LLEN(*ARG2);
    slen=LLEN(*ARG2)-1;
    if (mlen <= 0) {
        LZEROSTR(*ARGR);
        return;
    }
    LINITSTR(tabin);
    Lfx(&tabin,32);
    if (ARG3==NULL||LLEN(*ARG3)==0) {
        LLEN(tabin)=1;
        LSTR(tabin)Ý0¨=' ';
    } else {
        L2STR(ARG3);
        Lstrcpy(&tabin,ARG3);
    }

    LINITSTR(joins);
    Lfx(&joins, mlen);
    LLEN(joins)=mlen;

    L2STR(ARG1);
    LASCIIZ(*ARG1);
    L2STR(ARG2);
    LASCIIZ(*ARG2);

    for (i = 0; i < mlen; i++) {
        for (j = 0; j < LLEN(tabin); j++) {
            if (LSTR(*ARG2)Ýi¨ == LSTR(tabin)Ýj¨) goto joinChar;  // split char found             }
        }
        LSTR(joins)Ýi¨ = LSTR(*ARG2)Ýi¨;
        continue;
        joinChar:   LSTR(joins)Ýi¨ = LSTR(*ARG1)Ýi¨;
    }
    Lstrcpy(ARGR, &joins);
    LFREESTR(joins);
    LFREESTR(tabin);
}

void R_split(int func) {
    long i=0,j=0, n = 0, ctr=0;
    Lstr Word, tabin;
    char varNameÝ255¨;

    if (ARGN >3 || ARG1==NULL|| ARG2==NULL) Lerror(ERR_INCORRECT_CALL, 0);
    LINITSTR(tabin);
    Lfx(&tabin,32);
    if (ARG3==NULL||LLEN(*ARG3)==0) {
        LLEN(tabin)=1;
        LSTR(tabin)Ý0¨=' ';
    } else {
        L2STR(ARG3);
        Lstrcpy(&tabin,ARG3);
    }
    L2STR(ARG1);
    LASCIIZ(*ARG1);
    L2STR(ARG2);
    LASCIIZ(*ARG2);
    j=LLEN(*ARG2)-1;     // offset of last char
    if (LSTR(*ARG2)Ýj¨=='.') {
       LLEN(*ARG2)=j;
       LSTR(*ARG2)Ýj¨=NULL;
    }
    Lupper(ARG2);
    LINITSTR(Word);
    Lfx(&Word,LLEN(*ARG1)+1);

    bzero(varName, 255);
// Loop over provided string
    for (;;) {
 //    SKIP to next Word, Drop all word delimiter
          for (i = i; i < LLEN(*ARG1); i++) {
              for (j = 0; j < LLEN(tabin); j++) {
                  if (LSTR(*ARG1)Ýi¨ == LSTR(tabin)Ýj¨) goto splitChar;  // split char found             }
              }
              break;
              splitChar:
              continue;
          }
        dropChar: ;
        if (i>=LLEN(*ARG1)) break;
//    SKIP to next Delimiter, scan word
        for (n = i; n < LLEN(*ARG1); n++) {
            for (j = 0; j < LLEN(tabin); j++) {
                if (LSTR(*ARG1)Ýn¨ == LSTR(tabin)Ýj¨) goto splitCharf;  // split char found             }
            }
            continue;
            splitCharf:
            break;
        }
 //    Move Word into STEM
        ctr++;                    // Next word found, increase counter
        _Lsubstr(&Word,ARG1,i+1,n-i);
        LSTR(Word)Ýn-i¨=NULL;     // set 0 for end of string
        LLEN(Word)=n-i;
        sprintf(varName, "%s.%i",LSTR(*ARG2) ,ctr);
        setVariable(varName, LSTR(Word));  // set stem variable
        i=n;                      // newly set string offset for next loop
    }
//  set stem.0 content for found words
    sprintf(varName, "%s.0",LSTR(*ARG2));
    sprintf(LSTR(Word), "%ld", ctr);
    setVariable(varName, LSTR(Word));
    LFREESTR(Word);
    LFREESTR(tabin);
    Licpy(ARGR, ctr);   // return number if found words
}

void R_wait(int func)
{
    RX_WAIT_PARAMS_PTR params;
    void     *wk;
    unsigned time   = 0;
    int      cc     = 0;

    if (ARGN != 1)
        Lerror(ERR_INCORRECT_CALL,0);

    LASCIIZ(*ARG1);
    get_i (1,time);

    params = malloc(sizeof(RX_WAIT_PARAMS));
    wk     = malloc(256);

    params->timeadr      = &time;
    params->ccadr        = (unsigned *)&cc;
    params->wkadr        = (unsigned *)wk;

    call_rxwait(params);

    free(wk);
    free(params);
}

void R_abend(int func)
{
    RX_ABEND_PARAMS_PTR params;

    int ucc = 0;

    if (ARGN != 1)
        Lerror(ERR_INCORRECT_CALL,0);

    LASCIIZ(*ARG1);
    get_i (1,ucc);

    _setjmp_canc();

    params = malloc(sizeof(RX_ABEND_PARAMS));

    params->ucc          = ucc;

    call_rxabend(params);

    free(params);
}

void R_userid(int func)
{
    char *userid = "n.a.";

    if (ARGN>0) {
        Lerror(ERR_INCORRECT_CALL,0);
    }
#ifdef JCC
    userid = getlogin();
#endif
    Lscpy(ARGR,userid);
}

void R_listdsi(int func)
{
    char *argsÝ2¨;

    char sFileNameÝ45¨;
    char sFunctionCodeÝ3¨;

    FILE *pFile;
    int iErr;

    QuotationType quotationType;

    char* _style_old = _style;

    memset(sFileName,0,45);
    memset(sFunctionCode,0,3);

    iErr = 0;

    if (ARGN != 1)
        Lerror(ERR_INCORRECT_CALL,0);

    LASCIIZ(*ARG1);
    get_s(1);
    Lupper(ARG1);

    argsÝ0¨= NULL;
    argsÝ1¨= NULL;

    parseArgs(args, (char *)LSTR(*ARG1));

    if (argsÝ1¨ != NULL && strcmp(argsÝ1¨, "FILE") != 0)
        Lerror(ERR_INCORRECT_CALL,0);

    if (argsÝ1¨ == NULL) {
        _style = "//DSN:";
        quotationType = CheckQuotation(argsÝ0¨);
        switch (quotationType) {
            case UNQUOTED:
                if (environment->SYSPREFÝ0¨ != '\0') {
                    strcat(sFileName, environment->SYSPREF);
                    strcat(sFileName, ".");
                    strcat(sFileName, (const char *) LSTR(*ARG1));
                }
                break;
            case PARTIALLY_QUOTED:
                strcat(sFunctionCode, "16");
                iErr = 2;
                break;
            case FULL_QUOTED:
                strncpy(sFileName, (const char *) (LSTR(*ARG1)) + 1, ARG1->len - 2);
                break;
            default:
                Lerror(ERR_DATA_NOT_SPEC, 0);


        }
    } else {
        strcpy(sFileName,argsÝ0¨);
        _style = "//DDN:";
    }

    if (iErr == 0) {
        pFile = FOPEN(sFileName,"R");
        if (pFile != NULL) {
            strcat(sFunctionCode,"0");
            parseDCB(pFile);
            FCLOSE(pFile);
        } else {
            strcat(sFunctionCode,"16");
        }
    }

    Lscpy(ARGR,sFunctionCode);

    _style = _style_old;
}

void R_sysdsn(int func)
{
    char sDSNameÝ45¨;
    char sMessageÝ256¨;

    unsigned char *ptr;

    FILE *pFile;
    int iErr;

    QuotationType quotationType;

    char* _style_old = _style;

    const char* MSG_OK                  = "OK";
    const char* MSG_NOT_A_PO            = "MEMBER SPECIFIED, BUT DATASET IS NOT PARTITIONED";
    const char* MSG_MEMBER_NOT_FOUND    = "MEMBER NOT FOUND";
    const char* MSG_DATASET_NOT_FOUND   = "DATASET NOT FOUND";
    const char* MSG_ERROR_READING       = "ERROR PROCESSING REQUESTED DATASET";
    const char* MSG_DATSET_PROTECTED    = "PROTECTED DATASET";
    const char* MSG_VOLUME_NOT_FOUND    = "VOLUME NOT ON SYSTEM";
    const char* MSG_DATASET_UNAVAILABLE = "UNAVAILABLE DATASET";
    const char* MSG_INVALID_DSNAME      = "INVALID DATASET NAME, ";
    const char* MSG_MISSING_DSNAME      = "MISSING DATASET NAME";

    memset(sDSName,0,45);
    memset(sMessage,0,256);

    iErr = 0;

    if (ARGN != 1)
        Lerror(ERR_INCORRECT_CALL,0);

    LASCIIZ(*ARG1);
    get_s(1);
    Lupper(ARG1);

    if (LSTR(*ARG1)Ý0¨ == '\0') {
        strcat(sMessage,MSG_MISSING_DSNAME);
        iErr = 1;
    }

    if (iErr == 0) {
        quotationType = CheckQuotation((char *)LSTR(*ARG1));
        switch(quotationType) {
            case UNQUOTED:
                if (environment->SYSPREFÝ0¨ != '\0') {
                    strcat(sDSName, environment->SYSPREF);
                    strcat(sDSName, ".");
                    strcat(sDSName, (const char*)LSTR(*ARG1));
                }
                break;
            case PARTIALLY_QUOTED:
                strcat(sMessage,MSG_INVALID_DSNAME);
                strcat(sMessage,(const char*)LSTR(*ARG1));
                iErr = 2;
                break;
            case FULL_QUOTED:
                strncpy(sDSName, (const char *)(LSTR(*ARG1))+1, ARG1->len-2);
                break;
            default:
                Lerror(ERR_DATA_NOT_SPEC,0);
        }
    }

    if (iErr == 0) {
        _style = "//DSN:";
        pFile = FOPEN(sDSName,"R");
        if (pFile != NULL) {
            strcat(sMessage, MSG_OK);
            FCLOSE(pFile);
        } else {
            strcat(sMessage,MSG_DATASET_NOT_FOUND);
        }
    }

    Lscpy(ARGR,sMessage);

    _style = _style_old;
}

void R_sysvar(int func)
{
    extern unsigned long long ullInstrCount;
    char *msg = "not yet implemented";

    if (ARGN != 1) {
        Lerror(ERR_INCORRECT_CALL,0);
    }

    LASCIIZ(*ARG1);
    get_s(1);
    Lupper(ARG1);

    if (strcmp((const char*)ARG1->pstr, "SYSUID") == 0) {
        Lscpy(ARGR,environment->SYSUID);
    } else if (strcmp((const char*)ARG1->pstr, "SYSPREF") == 0) {
        Lscpy(ARGR, environment->SYSPREF);
    } else if (strcmp((const char*)ARG1->pstr, "SYSENV") == 0) {
        Lscpy(ARGR,environment->SYSENV);
    } else if (strcmp((const char*)ARG1->pstr, "SYSISPF") == 0) {
        Lscpy(ARGR, environment->SYSISPF);
    } else if (strcmp((const char*)ARG1->pstr, "RXINSTRC") == 0) {
        Licpy(ARGR, ullInstrCount);
    } else {
        Lscpy(ARGR,msg);
    }
}

void R_vxget(int func)
{
    PLstr plsValue;

    if (ARGN != 1) {
        Lerror(ERR_INCORRECT_CALL,0);
    }

    if ((environment->flags2 & _EXEC) == _EXEC &&
        (environment->flags2 & _ISPF) == _ISPF) {

        LPMALLOC(plsValue)
        LASCIIZ(*ARG1);

        get_s(1);
        Lupper(ARG1);

        GetClistVar(ARG1, plsValue);
        setVariable(LSTR(*ARG1), LSTR(*plsValue));

        LPFREE(plsValue);
    } else {
        Lerror(ERR_ROUTINE_NOT_FOUND,0);
    }
}

void R_vxput(int func)
{
    PLstr plsValue;

    if (ARGN != 1) {
        Lerror(ERR_INCORRECT_CALL,0);
    }

    if ((environment->flags2 & _EXEC) == _EXEC &&
        (environment->flags2 & _ISPF) == _ISPF) {

        LPMALLOC(plsValue)
        LASCIIZ(*ARG1);

        get_s(1);
        Lupper(ARG1);

        getVariable(LSTR(*ARG1), plsValue);
        SetClistVar(ARG1, plsValue);

        LPFREE(plsValue);
    } else {
        Lerror(ERR_ROUTINE_NOT_FOUND,0);
    }
}

void R_stemcopy(int func)
{
    BinTree *tree;
    PBinLeaf from, to, ptr ;
    Lstr tempKey, tempValue;
    Variable *varFrom, *varTo, *varTemp;

    if (ARGN!=2){
        Lerror(ERR_INCORRECT_CALL, 0);
    }

    // FROM
    Lupper(ARG1);
    LASCIIZ(*ARG1);

    // TO
    Lupper(ARG2);
    LASCIIZ(*ARG2);

    tree = _procÝ_rx_proc¨.scope;

    // look up Source stem
    from = BinFind(tree, ARG2);
    if (!from) {
        printf("Invalid Stem %s\n", LSTR(*ARG2));
        Lerror(ERR_INCORRECT_CALL,0);
    }

    //  look up Target stem, must be available, later set it up
    to = BinFind(tree, ARG1);
    if (!to) {
        printf("Target Stem missing %s\n", LSTR(*ARG1));
        Lerror(ERR_INCORRECT_CALL,0);
    }

    varFrom = (Variable *) from->value;
    varTo   = (Variable *) to->value;

    ptr = BinMin(varFrom->stem->parent);
    while (ptr != NULL) {

        LINITSTR(tempKey)
        LINITSTR(tempValue)

        Lstrcpy(&tempKey, &ptr->key);
        Lstrcpy(&tempValue, LEAFVAL(ptr));

        varTemp = (Variable *)MALLOC(sizeof(Variable),"Var");
        varTemp->value = tempValue;
        varTemp->exposed=((Variable *) ptr->value)->exposed;

        BinAdd((BinTree *)varTo->stem, &tempKey, varTemp);

        ptr = BinSuccessor(ptr);
    }

    LFREESTR(tempKey)
}

/* --------------------------------------------------------------- */
/*  DIR( file )                                                    */
/*    Exploiting Partitioned Data Set Directory Fields             */
/*      Part   I: http://www.naspa.net/magazine/1991/t9109019.txt  */
/*      Part  II: http://www.naspa.net/magazine/1991/t9110014.txt  */
/*      Part III: http://www.naspa.net/magazine/1991/t9111015.txt  */
/*    Using the System Status Index                                */
/*                http://www.naspa.net/magazine/1991/t9104004.txt  */
/* --------------------------------------------------------------- */
#define maxdirent 3000
#define endmark "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
#define UDL_MASK   ((int) 0x1F)
#define NPTR_MASK  ((int) 0x60)
#define ALIAS_MASK ((int) 0x80)

void __CDECL
R_dir( const int func )
{
    int iErr;

    long   ii;

    FILE * fh;

    char   recordÝ256¨;
    char   memberNameÝ8 + 1¨;
    char   aliasNameÝ8 + 1¨;
    char   ttrÝ6 + 1¨;
    char   versionÝ5 + 1¨;
    char   creationDateÝ8 + 1¨;
    char   changeDateÝ8 + 1¨;
    char   changeTimeÝ8 + 1¨;
    char   initÝ5 + 1¨;
    char   currÝ5 + 1¨;
    char   modÝ5 + 1¨;
    char   uidÝ8 + 1¨;

    unsigned char  *currentPosition;

    short  bytes;
    short  count;
    int    info_byte;
    short  numPointers;
    short  userDataLength;
    bool   isAlias;
    int    loadModuleSize;

    long   quit;
    short  l;
    char   sDSNÝ45¨;
    char   lineÝ255¨;
    char   *sLine;
    int    pdsecount = 0;

    P_USER_DATA pUserData;

    if (ARGN != 1) {
        Lerror(ERR_INCORRECT_CALL,0);
    }

    must_exist(1);
    get_s(1)

    LASCIIZ(*ARG1)

#ifndef __CROSS__
    Lupper(ARG1);
#endif

    bzero(sDSN, 45);

    _style = "//DSN:";

    // get the correct dsn for the input file
    iErr = getDatasetName(environment, (const char*)LSTR(*ARG1), sDSN);

    // open the pds directory
    fh = fopen (sDSN, "rb,klen=0,lrecl=256,blksize=256,recfm=u,force");

    if (fh != NULL) {
        // skip length field
        fread(&l, 1, 2, fh);

        quit = 0;

        while (fread(record, 1, 256, fh) == 256) {

            currentPosition = (unsigned char *) &(recordÝ2¨);
            bytes = ((short *) &(recordÝ0¨))Ý0¨;

            count = 2;
            while (count < bytes) {

                if (memcmp(currentPosition, endmark, 8) == 0) {
                    quit = 1;
                    break;
                }

                bzero(line, 255);
                sLine = line;

                bzero(memberName, 9);
                sprintf(memberName, "%.8s", currentPosition);
                {
                    // remove trailing blanks
                    long   jj = 7;
                    while (memberNameÝjj¨ == ' ') jj--;
                    memberNameÝ++jj¨ = 0;
                }
                sLine += sprintf(sLine, "%-8s", memberName);
                currentPosition += 8;   // skip current member name

                bzero(ttr, 7);
                sprintf(ttr, "%.2X%.2X%.2X", currentPositionÝ0¨, currentPositionÝ1¨, currentPositionÝ2¨);
                sLine += sprintf(sLine, "   %-6s", ttr);
                currentPosition += 3;   // skip ttr

                info_byte = (int) (*currentPosition);
                currentPosition += 1;   // skip info / stats byte

                numPointers    = (info_byte & NPTR_MASK);
                userDataLength = (info_byte & UDL_MASK) * 2;

                // no load lib
                if ( numPointers == 0 && userDataLength > 0) {
                    int year = 0;
                    int day  = 0;
                    char *datePtr;

                    pUserData = (P_USER_DATA) currentPosition;

                    bzero(version, 6);
                    sprintf(version, "%.2d.%.2d", pUserData->vlvl, pUserData->mlvl);
                    sLine += sprintf(sLine, " %-5s", version);

                    bzero(creationDate, 9);
                    datePtr = (char *) &creationDate;
                    year = getYear(pUserData->credtÝ0¨, pUserData->credtÝ1¨);
                    day  = getDay (pUserData->credtÝ2¨, pUserData->credtÝ3¨);
                    julian2gregorian(year, day, &datePtr);
                    sLine += sprintf(sLine, " %-8s", creationDate);

                    bzero(changeDate, 9);
                    datePtr = (char *) &changeDate;
                    year = getYear(pUserData->chgdtÝ0¨, pUserData->chgdtÝ1¨);
                    day  = getDay (pUserData->chgdtÝ2¨, pUserData->chgdtÝ3¨);
                    julian2gregorian(year, day, &datePtr);
                    sLine += sprintf(sLine, " %-8s", changeDate);

                    bzero(changeTime, 9);
                    sprintf(changeTime, "%.2x:%.2x:%.2x", (int)pUserData->chgtmÝ0¨, (int)pUserData->chgtmÝ1¨, (int)pUserData->chgss);
                    sLine += sprintf(sLine, " %-8s", changeTime);

                    bzero(init, 6);
                    sprintf(init, "%5d", pUserData->init);
                    sLine += sprintf(sLine, " %-5s", init);

                    bzero(curr, 6);
                    sprintf(curr, "%5d", pUserData->curr);
                    sLine += sprintf(sLine, " %-5s", curr);

                    bzero(mod, 6);
                    sprintf(mod, "%5d", pUserData->mod);
                    sLine += sprintf(sLine, " %-5s", mod);

                    bzero(uid, 9);
                    sprintf(uid, "%-.8s", pUserData->uid);
                    sLine += sprintf(sLine, " %-8s",  uid);
                } else {
                    isAlias        = (info_byte & ALIAS_MASK);

                    loadModuleSize = ((BYTE) *(currentPosition + 0xA)) << 16 |
                                     ((BYTE) *(currentPosition + 0xB)) << 8  |
                                     ((BYTE) *(currentPosition + 0xC));

                    sLine += sprintf(sLine, " %.6x", loadModuleSize);

                    if (isAlias) {
                        bzero(aliasName, 9);
                        sprintf(aliasName, "%.8s", currentPosition + 0x18);
                        {
                            // remove trailing blanks
                            long   jj = 7;
                            while (aliasNameÝjj¨ == ' ') jj--;
                            aliasNameÝ++jj¨ = 0;
                        }
                        sLine += sprintf(sLine, " %.8s", aliasName);
                    }
                }

                if (pdsecount == maxdirent) {
                    quit = 1;
                    break;
                } else {
                    char stemNameÝ13¨; // DIRENTRY (8) + . (1) + MAXDIRENTRY=3000 (4)
                    char varNameÝ32¨;

                    bzero(stemName, 13);
                    bzero(varName, 32);

                    sprintf(stemName, "DIRENTRY.%d", ++pdsecount);

                    sprintf(varName, "%s.NAME", stemName);
                    setVariable(varName, memberName);

                    sprintf(varName, "%s.TTR", stemName);
                    setVariable(varName, ttr);

                    if ((((info_byte & 0x60) >> 5) == 0) && userDataLength > 0) {
                        sprintf(varName, "%s.CDATE", stemName);
                        setVariable(varName, creationDate);

                        sprintf(varName, "%s.UDATE", stemName);
                        setVariable(varName, changeDate);

                        sprintf(varName, "%s.UTIME", stemName);
                        setVariable(varName, changeTime);

                        sprintf(varName, "%s.INIT", stemName);
                        setVariable(varName, init);

                        sprintf(varName, "%s.SIZE", stemName);
                        setVariable(varName, curr);

                        sprintf(varName, "%s.MOD", stemName);
                        setVariable(varName, mod);

                        sprintf(varName, "%s.UID", stemName);
                        setVariable(varName, uid);
                    }

                    sprintf(varName, "%s.LINE", stemName);
                    setVariable(varName, line);
                }

                currentPosition += userDataLength;

                count += (8 + 4 + userDataLength);
            }

            if (quit) break;

            fread(&l, 1, 2, fh); /* Skip U length */
        }

        fclose(fh);

        setIntegerVariable("DIRENTRY.0", pdsecount);
    }
}

void __CDECL
R_cputime( const int func )
{
    int rc = 0;

    char timeÝ16¨;
    char *sTime = time;

    bzero(time, 16);

    rc = cputime(&sTime);

    //Lscpy(ARGR, time);
    Licpy(ARGR, strtol(time, &sTime, 10));
}

// -------------------------------------------------------------------------------------
// Encrypt/Decrypt  String Sub procedure
// -------------------------------------------------------------------------------------
int _EncryptString(const PLstr to, const PLstr from, const PLstr password) {
    int slen,plen, ki, kj;
    L2STR(from);
    L2STR(password);
    slen=LLEN(*from);
    plen=LLEN(*password);
    for (ki = 0, kj=0; ki < slen; ki++,kj++) {
        if (kj >= plen) kj = 0;
        LSTR(*to)Ýki¨ = LSTR(*from)Ýki¨ ¬ LSTR(*password)Ýkj¨;
    }
    LLEN(*to) = (size_t) slen;
    LTYPE(*to) = LSTRING_TY;
    return slen;
}
// -------------------------------------------------------------------------------------
// Encrypt String
// -------------------------------------------------------------------------------------
void R_crypt(int func) {
    int rounds=7;
    // string to encrypt and password must exist
    must_exist(1);
    must_exist(2);
    get_oi0(3,rounds);       /* drop rounds parameter, it might decrease security */
    if (rounds==0) rounds=7;  /* maximum slots */
    Lcryptall(ARGR, ARG1, ARG2,rounds,0);  // mode =0  encode
}
// -------------------------------------------------------------------------------------
// Decrypt String
// -------------------------------------------------------------------------------------
void R_decrypt(int func) {
    int rounds=7;
    // string to encrypt and password must exist
    must_exist(1);
    must_exist(2);
    get_oi0(3,rounds);       /* drop rounds parameter, it might decrease security */
    if (rounds==0) rounds=7;  /* maximum slots */
    Lcryptall(ARGR, ARG1, ARG2,rounds,1); // mode =1  decode
}
// -------------------------------------------------------------------------------------
// Encrypt/Decrypt common Procedure
// -------------------------------------------------------------------------------------
void Lcryptall(PLstr to, PLstr from, PLstr pw, int rounds,int mode) {
    int plen, slen, ki,kj, hashv;
    Lstr pwt;
    L2STR(from);                 // make sure FROM is string
    L2STR(pw);                   // same for password
    slen = LLEN(*from);       // don't use STRLEN, as string may contain '0'x
    if (slen < 1) {              // is string empty? then return null string
        LZEROSTR(*to);
        return;
    }
    // set up temporary result
    Lfx(to, slen);
    Lstrcpy(to, from);
    // init Password definition
    plen = LLEN(*pw);
    if (plen == 0) return;   // no password given, string remains unchanged

    LINITSTR(pwt);
    Lfx(&pwt, plen);

    Lhash(&pwt, pw, 127);
    hashv = LINT(pwt);

    if (mode == 0) {  // encode
        // run through encryption in several rounds
        for (ki = 1; ki <= rounds; ki++) {    // Step 1: XOR String with Password
            for (kj = 0; kj < slen; kj++) {
                LSTR(*to)Ýkj¨ = LSTR(*to)Ýkj¨ + hashv;
            }
            hashv=(hashv+3)%127;
            _rotate(&pwt, pw, ki, 0);
            slen = _EncryptString(to, to, &pwt);
        }
    } else {    // decode
        hashv=(hashv+3*rounds-3)%127;
        for (ki = rounds; ki >= 1; ki--) {    // Step 1: XOR String with Password
            _rotate(&pwt, pw, ki,0);
            slen = _EncryptString(to, to, &pwt);
            for (kj = 0; kj < slen; kj++) {
                LSTR(*to)Ýkj¨=LSTR(*to)Ýkj¨-hashv;
            }
            hashv=(hashv-3)%127;
        }
    }
    // final settings and cleanup
    LLEN(*to) = (size_t) slen;
    LTYPE(*to) = LSTRING_TY;
    LFREESTR(pwt)
}

// -------------------------------------------------------------------------------------
// Rotate String
// -------------------------------------------------------------------------------------
// Return string at a certain position til it's end and continued substring before starting position
void _rotate(PLstr to, PLstr from, int start, int frlen) {
    int slen,rlen, istart=start,flen=frlen;

    slen=LLEN(*from);
    if (slen<1) {                  // is string empty? then return null string
        LZEROSTR(*to);
        return;
    }
    istart=istart%slen;             // if start > string length (re-calculate offset)
    istart--;                       // make start to a offset
    istart=istart%slen;             // if start > string length (re-calculate offset)
    rlen = slen- istart;            // lenght of remaining string
    if (flen==0) flen=slen;
    if (LISNULL(*to)) LINITSTR(*to);
    Lfx(to,slen);
// 1. copy remaining string part
    MEMMOVE( LSTR(*to), LSTR(*from)+istart, (size_t)rlen);
// 2. attach remaining length with string starting from position 1
    if (flen>rlen) MEMMOVE( LSTR(*to)+rlen, LSTR(*from), (size_t)slen-rlen);
    LLEN(*to) = (size_t) flen;
    LTYPE(*to) = LSTRING_TY;
}
// -------------------------------------------------------------------------------------
// Rotate String (registered stub)
// -------------------------------------------------------------------------------------
void R_rotate(int func) {
    int start, slen;
    must_exist(1);
    must_exist(2);
    get_oi(2,start);
    get_oi0(3,slen);
    _rotate(ARGR,ARG1,start,slen);
}
// -------------------------------------------------------------------------------------
// RHASH function
// -------------------------------------------------------------------------------------
void Lhash(const PLstr to, const PLstr from, long slots) {
    int ki,value=0, pcn,pwr,islots=INT32_MAX;
    size_t	lhlen=0;

    if (slots==0) slots=islots; /* maximum slots */

    pcn   = 71;                    /* potentially different Chars   */
    pwr = 1;                       /* Power of ... */

    if (!LISNULL(*from)) {
        switch (LTYPE(*from)) {
            case LINTEGER_TY:
                lhlen = sizeof(long);
                break;
            case LREAL_TY:
                lhlen = sizeof(double);
                break;
            case LSTRING_TY:
                lhlen = LLEN(*from);
                break;
        }

        for (ki = 0; ki < lhlen; ki++) {
            value = (value + (LSTR(*from)Ýki¨) * pwr)%islots;
            pwr = ((pwr * pcn) % islots);
        }
    }
    value=labs(value%slots);
    Licpy(to,labs(value));
}
<<<<<<< HEAD
// -------------------------------------------------------------------------------------
// RHASH (registered stub)
// -------------------------------------------------------------------------------------
=======

// TODO: TEST
typedef struct mtt_header {
    char tableId[4];
    void *current;
    void *start;
    void *end;
    int subPoolLen;
    char wrapTime[12];
    void *wrapPoint;
    void *reserver1;
    int dataLength;
    void *reserved2[21];
} MTT_HEADER, *P_MTT_HEADER;

typedef struct mtt_entry_header {
    short flags;
    short tag;
    void *immData;
    short len;
    unsigned char callerData;
} MTT_ENTRY_HEADER, *P_MTT_ENTRY_HEADER;

int updateIOPL (IOPL *iopl)
{
    int rc = 0;

    void **cppl;
    byte *ect;
    byte *ecb;
    byte *upt;

    // this stuf is TSO only
    if (!isTSO()) {
        return -1;
    }

    cppl = entry_R13[6];
    upt  = cppl[1];
    ect  = cppl[3];

    ((void **)iopl)[0] = upt;
    ((void **)iopl)[1] = ect;

    return 0;
}


/* ------------------------------------------------------------------------------------------------------------------ */

/* ---------------------------------------------------------------------------------------------------------------------
 * Thanks to Mike Carter, who helped with the correct ENQ Flags
 * ENQEXUNC EQU   X'40'  64   01000000     EXCLUSIVE UNCONDITIONAL.
 * ENQEXUSE EQU   X'43'  67   01100111     EXCLUSIVE RET=USE.
 * ENQEXTST EQU   X'47'  71   01110001     EXCLUSIVE RET=TEST.
 * ENQEXCHG EQU   X'42'  66   01000010     SHARED TO EXCLUSIVE.
 * ENQSHUNC EQU   X'C0'  192  11000000     SHARED UNCONDITIONAL.
 * ENQSHUSE EQU   X'C3'  195  11000011     SHARED RET=USE.
 * DEQUNC   EQU   X'41'  65   01000001     NORMAL DEQ(CONDITIONAL)
 * ENQDEQ   EQU   X'40'  64   01000000     NORMAL ENQ/DEQ INDICATION.
 * We use mainly:
 *   EXCLUSIVE mode=67
 *   SHARED    mode=195
 *   TEST      mode=71
 * for DEQ     mode=64
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */
void R_enq(int func)
{
    int inflags;
    RX_ENQ_PARAMS enq_parameter;
    RX_SVC_PARAMS svc_parameter;

    if (ARGN !=2) Lerror(ERR_INCORRECT_CALL, 0);

    LASCIIZ(*ARG1)
    Lupper(ARG1);
    get_s(1)
    get_i(2,inflags);

    enq_parameter.flags = 192; // List end Byte always 192 0xC= // 1100 0000
    enq_parameter.params = inflags;
    enq_parameter.rname = (char *) LSTR(*ARG1);
    enq_parameter.rname_length = LLEN(*ARG1);
    enq_parameter.ret = 0;
    enq_parameter.qname = "BREXX370";
    enq_parameter.rname = (char *) LSTR(*ARG1);

    svc_parameter.R1 = (uintptr_t) &enq_parameter;
    svc_parameter.SVC = 56;

    call_rxsvc(&svc_parameter);

    Licpy(ARGR, enq_parameter.ret);
}

/* ---------------------------------------------------------------------------------------------------------------------
 *   DEQ
 * ---------------------------------------------------------------------------------------------------------------------
 */
void R_deq(int func)
{
    bool test  = FALSE;
    bool block = FALSE;
    int inflags;

    RX_ENQ_PARAMS enq_parameter;
    RX_SVC_PARAMS svc_parameter;

    if (ARGN < 1 || ARGN > 2)  Lerror(ERR_INCORRECT_CALL, 0);

    LASCIIZ(*ARG1)
    Lupper(ARG1);
    get_s(1)
    get_i(2,inflags);

    enq_parameter.flags = 192; // 0xC= // 1100 0000
    enq_parameter.rname_length = LLEN(*ARG1);
    enq_parameter.params = inflags; // 0x49 // 0100 1001 / HAVE
    enq_parameter.ret = 0;
    enq_parameter.qname = "BREXX370";
    enq_parameter.rname = (char *) LSTR(*ARG1);

    svc_parameter.R1 = (uintptr_t) &enq_parameter;
    svc_parameter.SVC = 48;

    call_rxsvc(&svc_parameter);

    Licpy(ARGR, enq_parameter.ret);

}

void R_console(int func)
{
    RX_SVC_PARAMS svc_parameter;
    unsigned char cmd[128];

    if (ARGN !=1) Lerror(ERR_INCORRECT_CALL, 0);

    LASCIIZ(*ARG1)
    Lupper(ARG1);
    get_s(1)

    privilege(1);
    bzero(cmd, sizeof(cmd));
    cmd[1] = 104;

    memset(&cmd[4], 0x40, 124);
    memcpy(&cmd[4], LSTR(*ARG1), LLEN(*ARG1));

    /* SEND COMMAND */
    svc_parameter.R0 = (uintptr_t) 0;
    svc_parameter.R1 = (uintptr_t) &cmd[0];
    svc_parameter.SVC = 34;
    call_rxsvc(&svc_parameter);

    privilege(0);
}

void R_privilege(int func) {
    int rc = 8;

    RX_SVC_PARAMS svc_parameter;

    if (ARGN != 1)
        Lerror(ERR_INCORRECT_CALL, 0);   // then NOP;

    /*
    if (!rac_check(FACILITY, PRIVILAGE, READ)) {
        RxSetSpecialVar(RCVAR, -3);
        return;
    }
    */

    LASCIIZ(*ARG1)
    Lupper(ARG1);
    get_s(1)

    if (strcmp((const char *) ARG1->pstr, "ON") == 0) {
        rc = privilege(1);
    } else if (strcmp((const char *) ARG1->pstr, "OFF") == 0) {
        rc = privilege(0);
    }

    Licpy(ARGR, rc);
}

void R_error(int func) {
    if (ARGN != 1)
        Lerror(ERR_INCORRECT_CALL,0);
    LASCIIZ(*ARG1)
    Lupper(ARG1);
    get_s(1)
    Lfailure(LSTR(*ARG1),"","","","");
}

void R_getg(int func)
{
    PLstr tmp;

    if (ARGN != 1)
        Lerror(ERR_INCORRECT_CALL,0);

    LASCIIZ(*ARG1)
    Lupper(ARG1);
    get_s(1)

    tmp = hashMapGet(globalVariables, (char *) LSTR(*ARG1));

    if (tmp && !LISNULL(*tmp)) {
        Lstrcpy(ARGR, tmp);
    } else {
        LZEROSTR(*ARGR)
    }
}

void R_setg(int func)
{
    PLstr pValue;

    if (ARGN != 2)
        Lerror(ERR_INCORRECT_CALL,0);

    LASCIIZ(*ARG1)
    Lupper(ARG1);
    get_s(1)

    LPMALLOC(pValue)
    Lstrcpy(pValue, ARG2);

    hashMapSet(globalVariables, (char *) LSTR(*ARG1), pValue);

    Lstrcpy(ARGR, ARG2);
}

void R_level(int func) {
    int level,nlevel;
    RxProc	*pr = &(_proc[_rx_proc]);

    if (ARGN>0) {
        level = (int) Lrdint(ARG1);

        if (level <= 0) {
            nlevel = _rx_proc + level;
            if (nlevel < 0) nlevel = 0;
        } else {
            if (level > _rx_proc) nlevel = _rx_proc;
            else nlevel = level;
        }
        pr = &(_proc[nlevel]);
        printf("level %d \n",nlevel);

        return ;
    }

    Licpy(ARGR,_rx_proc);
}

/* -------------------------------------------------------------- */
/*  ARG([n[,option]])                                             */
/* -------------------------------------------------------------- */
void R_argv(int func)
{
    int	pnum,level,nlevel,error ;

    RxProc	*pr = &(_proc[_rx_proc]);

    if (ARGN>1) level = (int)Lrdint(ARG2);
    else level=-1;

    if (level<=0) {
        nlevel = _rx_proc + level;
        if (nlevel < 0) nlevel = 0;
    } else {
        if (level > _rx_proc) nlevel=_rx_proc;
        else nlevel=level;
    }
    pr = &(_proc[nlevel]);

    get_oiv(1, pnum, 0)
    if (pnum==0) {
       Licpy(ARGR, pr->arg.n);
       return;
    }
    if (pnum>pr->arg.n) LZEROSTR(*ARGR)
    else Lstrcpy(ARGR, pr->arg.a[pnum - 1]);
 } /* R_arg */

/* ------------------------------------------------------------------------------------
 * Pick exactly one CHAR out of a string
 * ------------------------------------------------------------------------------------
 */
void R_char(int func) {
    char pad;
    int cnum;
    Lfx(ARGR,8);
    get_s(1);
    get_i(2,cnum);
    get_pad(3,pad);
    if (cnum<=LLEN(*ARG1)) pad=LSTR(*ARG1)[cnum-1];
    Lscpy(ARGR,&pad);
    LLEN(*ARGR)=1;
}

/* ------------------------------------------------------------------------------------
 * DateTime Main function
 * ------------------------------------------------------------------------------------
 */
void R_dattimbase(int func) {
    int dnum = 0;
    char imod, omod;

    if (ARG1==NULL) omod=' ';
    else {
        Lupper(ARG1);
        omod=LSTR(*ARG1)[0];
    }

    if (ARG3==NULL) imod=' ';
    else {
        Lupper(ARG3);
        imod=LSTR(*ARG3)[0];
    }

    if (imod == 'T' && _Lisnum(ARG2) != LINTEGER_TY)  Lfailure("invalid Date/in-format combination",LSTR(*ARG2),"/",&imod,"");
    if (imod==omod) {
        if (ARG2==NULL ) dnum=1;
        if (dnum==0 && LLEN(*ARG2)==0) dnum=1;
        if (dnum==1) Lfailure("Empty Date field","","","","");
        if (imod == 'T')  {
            Lstrcpy(ARGR,ARG2);
            return;
        }
        datetimebase(ARGR, 'T', ARG2, imod);
        imod = 'T';
    } else if (ARG2==NULL || LLEN(*ARG2)==0) {
        datetimebase(ARGR, 'T', NULL, 'B');
        if (omod == 'T') return;
        imod = 'T';
    } else Lstrcpy(ARGR,ARG2);

    datetimebase(ARGR,omod, ARGR, imod);
}

void R_outtrap(int func)
{
    int rc =0;

    RX_TSO_PARAMS  tso_parameter;
    void ** cppl;

    __dyn_t dyn_parms;

    if (ARGN < 1 || ARGN > 4) {
        Lerror(ERR_INCORRECT_CALL, 0);
    }

    if (isTSO()!= 1 ||  entry_R13 [6] == 0) {
        Lerror(ERR_INCORRECT_CALL, 0);
    }

    if (exist(1)) {
        get_s(1);
        LASCIIZ(*ARG1);
    }

    if (exist(2)) {
        if(LTYPE(*ARG2) == LINTEGER_TY) {
            outtrapCtx->maxLines = LINT(*ARG2);
        }
    }

    if (exist(3)) {
        get_s(3);
        LASCIIZ(*ARG1);
        if (strcasecmp("NOCONCAT", (const char *) LSTR(*ARG3)) == 0) {
            outtrapCtx->concat = FALSE;
        }
    }

    if (exist(4)) {
        if (LTYPE(*ARG4) == LINTEGER_TY) {
            outtrapCtx->skipAmt = LINT(*ARG4);
            if (outtrapCtx->skipAmt > 999999999) {
                outtrapCtx->skipAmt = 999999999;
            }
        }
    }

    cppl = entry_R13[6];

    memset(&tso_parameter, 00, sizeof(RX_TSO_PARAMS));
    tso_parameter.cppladdr = (unsigned int *) cppl;

    if (strcasecmp("OFF", (const char *) LSTR(*ARG1)) != 0) {
        // remember variable name
            memset(&outtrapCtx->varName,0,sizeof(&outtrapCtx->varName));
            Lstrcpy(&outtrapCtx->varName, ARG1);

        dyninit(&dyn_parms);
        dyn_parms.__ddname    = (char *) LSTR(outtrapCtx->ddName);
        dyn_parms.__status    = __DISP_NEW;
        dyn_parms.__unit      = "VIO";
        dyn_parms.__dsorg     = __DSORG_PS;
        dyn_parms.__recfm     = _FB_;
        dyn_parms.__lrecl     = 133;
        dyn_parms.__blksize   = 13300;
        dyn_parms.__alcunit   = __TRK;
        dyn_parms.__primary   = 5;
        dyn_parms.__secondary = 5;

        rc = dynalloc(&dyn_parms);

        strcpy(tso_parameter.ddout, (const char *) LSTR(outtrapCtx->ddName));

        rc = call_rxtso(&tso_parameter);

    } else {
        rc = call_rxtso(&tso_parameter);

        rc = get2variables(&outtrapCtx->varName, &outtrapCtx->ddName,
                           outtrapCtx->maxLines, outtrapCtx->concat,
                           outtrapCtx->skipAmt);

        dyninit(&dyn_parms);
        dyn_parms.__ddname = (char *) LSTR(outtrapCtx->ddName);
        rc = dynfree(&dyn_parms);
    }

    Licpy(ARGR, rc);
}

void R_dumpIt(int func)
{
    void *ptr  = 0;
    int   size = 0;
    long  adr  = 0;

    if (ARGN > 2 || ARGN < 1) {
        Lerror(ERR_INCORRECT_CALL,0);
    }

    if (ARGN == 1) {

    } else {
        Lx2d(ARGR,ARG1,0);    /* using ARGR as temp field for conversion */
        adr = Lrdint(ARGR);
        if (adr < 0) {
            Lerror(ERR_INCORRECT_CALL, 0);
        }

        ptr = (void *)adr;
        size = Lrdint(ARG2);
    }



    DumpHex((unsigned char *)ptr, size);
}

void R_wto(int func)
{
    int msgId = 0;

    if (ARGN != 1)
        Lerror(ERR_INCORRECT_CALL,0);

    LASCIIZ(*ARG1);
    get_s(1);

    msgId = _write2op((char *)LSTR(*ARG1));

    LICPY(*ARGR, msgId);
}

void R_listIt(int func)
{
    BinTree tree;
    int	j;
    if (ARGN > 1 ) {
        Lstr lsFuncName,lsMaxArg;

        LINITSTR(lsFuncName)
        LINITSTR(lsMaxArg)

        Lfx(&lsFuncName,6);
        Lfx(&lsMaxArg, 4);

        Lscpy(&lsFuncName, "ListIT");
        Licpy(&lsMaxArg,1);

        Lerror(ERR_INCORRECT_CALL,4,&lsFuncName, &lsMaxArg);
    }

    if (ARG1 != NULL && ARG1->pstr == NULL) {
        printf("LISTIT: invalid parameters, maybe enclose in quotes\n");
        Lerror(ERR_INCORRECT_CALL,4,1);
    }

    tree = _proc[_rx_proc].scope[0];

    if (ARG1 == NULL || LSTR(*ARG1)[0] == 0) {
        printf("List all Variables\n");
        printf("------------------\n");
        BinPrint(tree.parent, NULL);
    } else {
        LASCIIZ(*ARG1) ;
        Lupper(ARG1);
        printf("List Variables with Prefix '%s'\n",ARG1->pstr);
        printf("%.*s\n", 29+ARG1->len,
               "-------------------------------------------------------");
        BinPrint(tree.parent, ARG1);
    }
}

void R_vlist(int func)
{
    BinTree tree;
    int	j,found=0;
    int mode=1;
    get_s(1);
    LASCIIZ(*ARG1);

    if (ARGN > 3 ) {
        Lstr lsFuncName,lsMaxArg;

        LINITSTR(lsFuncName)
        LINITSTR(lsMaxArg)

        Lfx(&lsFuncName,5);
        Lfx(&lsMaxArg, 4);

        Lscpy(&lsFuncName, "VList");
        Licpy(&lsMaxArg,2);

        Lerror(ERR_INCORRECT_CALL,4,&lsFuncName, &lsMaxArg);
    }

    if (ARG1 != NULL && ARG1->pstr == NULL)  Lfailure("VLIST: invalid parameters, maybe enclose in quotes","","","","");
    if (exist(2)) {
        get_s(2);
        LASCIIZ(*ARG2);
        Lupper(ARG2);
        if (LSTR(*ARG2)[0] == 'V') mode = 1;
        else if (LSTR(*ARG2)[0] == 'N') mode = 2;
        else if (LSTR(*ARG2)[0] == 'A') {
            if (exist(3)){
                get_s(3);
                LASCIIZ(*ARG3);
                Lupper(ARG3);
                if (LSTR(*ARG1)[LLEN(*ARG1)-1]!='.')  Lfailure("AS Clause only available for STEM variables:",LSTR(*ARG1),"","","");
                if (LSTR(*ARG3)[LLEN(*ARG3)-1]!='.')  Lfailure("AS Clause must be STEM variable:",LSTR(*ARG3),"","","");
                mode = 3;      // AS Clause
            }
        }
    }

    tree = _proc[_rx_proc].scope[0];

    if (ARG1 == NULL || LSTR(*ARG1)[0] == 0) {
        found=BinVarDump(ARGR,tree.parent, NULL,mode,ARG3);
    } else {
        Lstr argone;
        LINITSTR(argone);
        Lfx(&argone, LLEN(*ARG1));
        Lstrcpy(&argone, ARG1);
        Lupper(&argone);
        if (LSTR(argone)[LLEN(argone) - 1] == '.') {
            strcat(LSTR(argone), "*");
            LLEN(argone)= LLEN(argone) + 1;
        }
        found=BinVarDump(ARGR, tree.parent, &argone, mode, ARG3);
        LFREESTR(argone);
    }
    setIntegerVariable("VLIST.0", found);
}

void R_stemhi(int func)
{
    BinTree tree;
    int	found=0;

    if (ARGN !=1)  Lerror(ERR_INCORRECT_CALL,4,1);

    if (ARG1 == NULL || LSTR(*ARG1)[0] == 0) {
        // NOP
    } else {
        LASCIIZ(*ARG1) ;
        Lupper(ARG1);
        if (LSTR(*ARG1)[LLEN(*ARG1)-1]!='.') {
            strcat(LSTR(*ARG1),".");
            LLEN(*ARG1)=LLEN(*ARG1)+1;
        }
        tree = _proc[_rx_proc].scope[0];
        found=BinStemCount(ARGR,tree.parent, ARG1);
    }
    Licpy(ARGR ,found);
}

void arginas(PLstr isname, const char* asname) {
    Lstrcpy(ARG1, isname);  // replace it by requested as-name

    R_vlist(0);                // search for all variables returned is set-list with all entries
}

void R_argin(int func) {
    BinTree tree;
    int stemi,vlist=0,rc;
    RxProc *pr;
    PBinLeaf	litleaf;

    get_i(1,stemi)
    pr = &(_proc[_rx_proc]);   // current proc level
    if(stemi>pr->arg.n) Licpy(ARGR,rc);
    else {
         Lstrcpy(ARGR, pr->arg.a[stemi - 1]);  // copy requested variable name
         Lupper(ARGR);
 //        tree = _proc[_rx_proc - 1].scope[0];          // set to caller level
         litleaf = BinFind(&rxLitterals, ARGR);
         if (litleaf) {
             RxVarExpose(_proc[_rx_proc].scope, litleaf);
             if exist(3) {
                get_sv(3)
                arginas(ARGR, LSTR(*ARG3));
             }
            } else Licpy(ARGR,rc);
     }
    Licpy(ARG1,stemi);
 }

void R_bldl(int func) {
    int found=0;
    if (ARGN != 1 || LLEN(*ARG1)==0) Lerror(ERR_INCORRECT_CALL,0);
    LASCIIZ(*ARG1) ;
    Lupper(ARG1);

    if (findLoadModule((char *)LSTR(*ARG1))) found=1;
    Licpy(ARGR,found);
}

void R_upper(int func) {
    if (ARGN != 1) Lerror(ERR_INCORRECT_CALL,0);

    if (LTYPE(*ARG1) != LSTRING_TY) {
        L2str(ARG1);
    };
    LASCIIZ(*ARG1) ;
    Lstrcpy(ARGR,ARG1);
    Lupper(ARGR);
}

void R_lower(int func) {
    if (ARGN != 1) Lerror(ERR_INCORRECT_CALL,0);

    if (LTYPE(*ARG1) != LSTRING_TY) {
        L2str(ARG1);
    };
    LASCIIZ(*ARG1) ;
    Lstrcpy(ARGR,ARG1);
    Llower(ARGR);
}

void R_lastword(int func) {
    long	offset=0, lwi=0, lwe=0,wrds;

    LZEROSTR(*ARGR);   // default no word

    if (LLEN(*ARG1)==0) return;

    get_sv(1);
    get_oiv(2,wrds,1)

    offset= LLEN(*ARG1) - 1;


    while (wrds>0) {
        while (offset >= 0 && ISSPACE(LSTR(*ARG1)[offset])) offset--;
        if (offset < 0) break;
        lwe = offset + 2; // offset points to last char of word +1 to place it to next blank, +1 to make offset to position

        while (offset >= 0 && !ISSPACE(LSTR(*ARG1)[offset])) offset--;
        lwi= offset + 2;   // offset points to first blank prior to word +1 to place it to first char of word, +1 to make offset to position
        wrds--;
    }
     if (wrds==0) _Lsubstr(ARGR,ARG1,lwi,lwe-lwi);
}

void R_join(int func) {
    int mlen = 0, slen=0, i = 0,j=0;
    Lstr joins, tabin;
    if (ARGN >3 || ARGN<2 || ARG1==NULL || ARG2==NULL) Lerror(ERR_INCORRECT_CALL, 0);
    if (LLEN(*ARG1) <1) {
        Lstrcpy(ARGR, ARG2);
        return;
    }
    if (LLEN(*ARG2) <1) {
        Lstrcpy(ARGR, ARG1);
        return;
    }
    if (LLEN(*ARG1) > LLEN(*ARG2)) mlen = LLEN(*ARG1);
    else mlen = LLEN(*ARG2);
    slen=LLEN(*ARG2)-1;
    if (mlen <= 0) {
        LZEROSTR(*ARGR);
        return;
    }
    LINITSTR(tabin);
    Lfx(&tabin,32);
    if (ARG3==NULL||LLEN(*ARG3)==0) {
        LLEN(tabin)=1;
        LSTR(tabin)[0]=' ';
    } else {
        L2STR(ARG3);
        Lstrcpy(&tabin,ARG3);
    }

    LINITSTR(joins);
    Lfx(&joins, mlen);
    LLEN(joins)=mlen;

    L2STR(ARG1);
    LASCIIZ(*ARG1);
    L2STR(ARG2);
    LASCIIZ(*ARG2);

    for (i = 0; i < mlen; i++) {
        for (j = 0; j < LLEN(tabin); j++) {
            if (LSTR(*ARG2)[i] == LSTR(tabin)[j]) goto joinChar;  // split char found             }
        }
        LSTR(joins)[i] = LSTR(*ARG2)[i];
        continue;
        joinChar:   LSTR(joins)[i] = LSTR(*ARG1)[i];
    }
    Lstrcpy(ARGR, &joins);
    LFREESTR(joins);
    LFREESTR(tabin);
}

void R_split(int func) {
    long i=0,j=0, n = 0, ctr=0;
    char varName[255];
    int sdot=0;

    if (ARGN >3 || ARG1==NULL|| ARG2==NULL) Lerror(ERR_INCORRECT_CALL, 0);
    if (ARG3==NULL||LLEN(*ARG3)==0) {
        LLEN(LTMP[14])=1;
        LSTR(LTMP[14])[0]=' ';
    } else {
        L2STR(ARG3);
        Lstrcpy(&LTMP[14], ARG3);
    }
    L2STR(ARG1);
    LASCIIZ(*ARG1);
    L2STR(ARG2);
    LASCIIZ(*ARG2);
    j=LLEN(*ARG2)-1;     // offset of last char
    if (LSTR(*ARG2)[j]=='.') sdot=1;  // address target stem variable
    Lupper(ARG2);

    bzero(varName, 255);
// Loop over provided string
    for (;;) {
        //    SKIP to next Word, Drop all word delimiter
        for (i = i; i < LLEN(*ARG1); i++) {
            for (j = 0; j < LLEN(LTMP[14]); j++) {
                if (LSTR(*ARG1)[i] == LSTR(LTMP[14])[j]) goto splitChar;  // split char found             }
            }
            break;
            splitChar:
            continue;
        }
        dropChar: ;
        if (i>=LLEN(*ARG1)) break;
//    SKIP to next Delimiter, scan word
        for (n = i; n < LLEN(*ARG1); n++) {
            for (j = 0; j < LLEN(LTMP[14]); j++) {
                if (LSTR(*ARG1)[n] == LSTR(LTMP[14])[j]) goto splitCharf;  // split char found             }
            }
            continue;
            splitCharf:
            break;
        }
        //    Move Word into STEM
        ctr++;                    // Next word found, increase counter
        _Lsubstr(&LTMP[15], ARG1, i + 1, n - i);
        LSTR(LTMP[15])[n - i]=0;     // set 0 for end of string
        LLEN(LTMP[15])= n - i;
        if (sdot==0) sprintf(varName, "%s.%i",LSTR(*ARG2) ,ctr);
        else sprintf(varName, "%s%i",LSTR(*ARG2) ,ctr);
        setVariable(varName, LSTR(LTMP[15]));  // set stem variable
        i=n;                      // newly set string offset for next loop
    }
//  set stem.0 content for found words
    if (sdot==0) sprintf(varName, "%s.0",LSTR(*ARG2));
    else sprintf(varName, "%s0",LSTR(*ARG2));
    sprintf(LSTR(LTMP[15]), "%ld", ctr);
    setVariable(varName, LSTR(LTMP[15]));
    Licpy(ARGR, ctr);   // return number if found words
}

void R_wait(int func)
{
    int val;

    time_t seconds;

    if (ARGN != 1)
        Lerror(ERR_INCORRECT_CALL,0);

    LASCIIZ(*ARG1);
    get_i (1,val);

    Sleep(val);
}

void R_abend(int func)
{
    RX_ABEND_PARAMS_PTR params;

    int ucc = 0;

    if (ARGN != 1)
        Lerror(ERR_INCORRECT_CALL,0);

    LASCIIZ(*ARG1);
    get_i (1,ucc);

    if (ucc < 1 || ucc > 3999)
        Lerror(ERR_INCORRECT_CALL,0);

    _setjmp_ecanc();

    params = MALLOC(sizeof(RX_ABEND_PARAMS), "R_abend_parms");

    params->ucc          = ucc;

    call_rxabend(params);

    FREE(params);
}

void R_userid(int func)
{
    char *userid = "n.a.";

    if (ARGN > 0) {
        Lerror(ERR_INCORRECT_CALL,0);
    }
#ifdef JCC
    userid = getlogin();
#endif
    Lscpy(ARGR, userid);
}

void PDSdet (char * filename)
{
    int info_byte, memi=0,diri=0,flen=0;
    short l, bytes, count, userDataLength;
    FILE *fh;
    char record[256];
    unsigned char *currentPosition;

    fh = fopen((const char *) filename, "rb,klen=0,lrecl=256,blksize=256,recfm=u,force");
    if (fh == NULL) return;
    // skip length field
    fread(&l, 1, 2, fh);
    while (fread(record, 1, 256, fh) == 256) {
        currentPosition = (unsigned char *) &(record[2]);
        bytes = ((short *) &(record[0]))[0];
        count = 2;
        diri++;
        while (count < bytes) {
            if (memcmp(currentPosition, endmark, 8) == 0) goto leaveAll;
            memi++;
            currentPosition += 11;   // skip current member name + ttr
            info_byte = (short) (*currentPosition);
            currentPosition += 1;
            userDataLength = (info_byte & UDL_MASK) * 2;
            currentPosition += userDataLength;
            count += (8 + 4 + userDataLength);
        }
        fread(&l, 1, 2, fh); /* Skip U length */
    }
    leaveAll:
    setIntegerVariable("SYSDIRBLK",diri);
    setIntegerVariable("SYSMEMBERS",memi);
    if (fseek(fh, 0, SEEK_END) == 0) flen = ftell(fh);
    setIntegerVariable("SYSSIZE2", flen);
    setIntegerVariable("SYSSIZE", flen);
    setVariable("SYSRECORDS","n/a");
    fclose(fh);


}

void R_listdsi(int func)
{
    char *args[2];

    char sFileName[45];
    char sFunctionCode[3];

    FILE *pFile;
    int flen=0,po=0,recfm=0,lrecl=0;
    char sflen[9],dsorg[6];
    int iErr;

    QuotationType quotationType;

    char* _style_old = _style;

    memset(sFileName,0,45);
    memset(sFunctionCode,0,3);

    iErr = 0;

    if (ARGN != 1)
        Lerror(ERR_INCORRECT_CALL,0);

    LASCIIZ(*ARG1);
    get_s(1);
    Lupper(ARG1);

    args[0]= NULL;
    args[1]= NULL;

    parseArgs(args, (char *)LSTR(*ARG1));

    if (args[1] != NULL && strcmp(args[1], "FILE") != 0)
        Lerror(ERR_INCORRECT_CALL,0);

    if (args[1] == NULL) {
        _style = "//DSN:";
        quotationType = CheckQuotation(args[0]);
        switch (quotationType) {
            case UNQUOTED:
                if (environment->SYSPREF[0] != '\0') {
                    strcat(sFileName, environment->SYSPREF);
                    strcat(sFileName, ".");
                    if (LLEN(*ARG1)+strlen(sFileName)>44){
                       printf("DSN exceeds 44 characters, requested length: %d \n",LLEN(*ARG1)+strlen(sFileName));
                       iErr=3;
                    } else strcat(sFileName, (const char *) LSTR(*ARG1));
                }
                break;
            case PARTIALLY_QUOTED:
                strcat(sFunctionCode, "16");
                iErr = 2;
                break;
            case FULL_QUOTED:
                if (LLEN(*ARG1)>46){
                    printf("DSN exceeds 44 characters, requested length: %d\n",LLEN(*ARG1)-2);
                    iErr=3;
                } else strncpy(sFileName, (const char *) (LSTR(*ARG1)) + 1, ARG1->len - 2);
                break;
            default:
                Lerror(ERR_DATA_NOT_SPEC, 0);

        }
    } else {
        if (LLEN(*ARG1)>8){
            printf("DD name exceeds 8 characters, requested length: %d\n",LLEN(*ARG1));
            iErr=4;
        } else {
            strcpy(sFileName, args[0]);
            _style = "//DDN:";
        }
    }

    if (iErr == 0) {
        char pbuff[4096];
        int records=0;
        pFile = FOPEN(sFileName,"R");
           if (pFile != NULL) {
              strcat(sFunctionCode,"0");
              po=parseDCB(pFile);
              recfm=po/10;   // select recfm F or FB
              po=po%10;      // partititioned or SEQ
              if (po!=1) {  // po=0 PS, p0=1 PDS with assigned member, is treated as sequential
                  while (fgets(pbuff, 4096, pFile)) records++;  // just read 3 bytes to be safe including CRLF
                  setIntegerVariable("SYSRECORDS",records);
                  if (fseek(pFile, 0, SEEK_END) == 0) flen = ftell(pFile);
                  setIntegerVariable("SYSSIZE2",flen);
                  lrecl=getIntegerVariable("SYSLRECL");
                  if (recfm>0) setIntegerVariable("SYSSIZE",records*lrecl);
                      else setIntegerVariable("SYSSIZE",flen);
                  setVariable("SYSDIRBLK","n/a");
                  setVariable("SYSMEMBERS","n/a");
              }
              FCLOSE(pFile);
              if (po==1) {
                 PDSdet(sFileName);
              }
        } else {
            strcat(sFunctionCode,"16");
        }
    }

    Lscpy(ARGR,sFunctionCode);

    _style = _style_old;
}
/* ----------------------------------------------------------------------------
 * LISTDSIQ fast version with limited attributes
 *     fully qualified dsn expected (no FILE variant), no quotes are allowed
 * ----------------------------------------------------------------------------
 */
void R_listdsiq(int func)
{
    char sFileName[45];
    char sFunctionCode[3];
    char mode='N';
    char pbuff[4096];

    FILE *pFile;
    int iErr,records=0;

    QuotationType quotationType;

    char* _style_old = _style;

    memset(sFileName,0,45);
    memset(sFunctionCode,0,3);

    iErr = 0;

    if (ARGN >2) Lerror(ERR_INCORRECT_CALL,0);

    LASCIIZ(*ARG1);
    get_s(1);
    Lupper(ARG1);

    get_sv(2);
    if (ARGN==2) mode=LSTR(*ARG2)[0];

    _style = "//DSN:";
    if (LLEN(*ARG1)>44){
        printf("DSN exceeds 44 characters, requested length: %d\n",LLEN(*ARG1)-2);
        iErr=3;
    } else strcpy(sFileName, (const char *) (LSTR(*ARG1)));

    if (iErr == 0) {
        pFile = FOPEN(sFileName,"R");
        if (pFile != NULL) {
            parseDCB(pFile);
            if (mode=='R'){
               while (fgets(pbuff, 4096, pFile)) records++;
               setIntegerVariable("SYSRECORDS",records);
            }
            FCLOSE(pFile);
            iErr = 0;
        } else iErr=16;
    }
    Licpy(ARGR,iErr);
    _style = _style_old;
}

void R_sysdsn(int func)
{
    char sDSName[45];
    char sMessage[256];

    unsigned char *ptr;

    FILE *pFile;
    int iErr;

    QuotationType quotationType;

    char* _style_old = _style;

    const char* MSG_OK                  = "OK";
    const char* MSG_NOT_A_PO            = "MEMBER SPECIFIED, BUT DATASET IS NOT PARTITIONED";
    const char* MSG_MEMBER_NOT_FOUND    = "MEMBER NOT FOUND";
    const char* MSG_DATASET_NOT_FOUND   = "DATASET NOT FOUND";
    const char* MSG_ERROR_READING       = "ERROR PROCESSING REQUESTED DATASET";
    const char* MSG_DATSET_PROTECTED    = "PROTECTED DATASET";
    const char* MSG_VOLUME_NOT_FOUND    = "VOLUME NOT ON SYSTEM";
    const char* MSG_DATASET_UNAVAILABLE = "UNAVAILABLE DATASET";
    const char* MSG_INVALID_DSNAME      = "INVALID DATASET NAME, ";
    const char* MSG_MISSING_DSNAME      = "MISSING DATASET NAME";

    memset(sDSName,0,45);
    memset(sMessage,0,256);

    iErr = 0;

    if (ARGN != 1)
        Lerror(ERR_INCORRECT_CALL,0);

    LASCIIZ(*ARG1);
    get_s(1);
    Lupper(ARG1);

    if (LSTR(*ARG1)[0] == '\0') {
        strcat(sMessage,MSG_MISSING_DSNAME);
        iErr = 1;
    }

    if (iErr == 0) {
        quotationType = CheckQuotation((char *)LSTR(*ARG1));
        switch(quotationType) {
            case UNQUOTED:
                if (environment->SYSPREF[0] != '\0') {
                    strcat(sDSName, environment->SYSPREF);
                    strcat(sDSName, ".");
                    strcat(sDSName, (const char*)LSTR(*ARG1));
                }
                break;
            case PARTIALLY_QUOTED:
                strcat(sMessage,MSG_INVALID_DSNAME);
                strcat(sMessage,(const char*)LSTR(*ARG1));
                iErr = 2;
                break;
            case FULL_QUOTED:
                strncpy(sDSName, (const char *)(LSTR(*ARG1))+1, ARG1->len-2);
                break;
            default:
                Lerror(ERR_DATA_NOT_SPEC,0);
        }
    }

    if (iErr == 0) {
        _style = "//DSN:";
        pFile = FOPEN(sDSName,"R");
        if (pFile != NULL) {
            strcat(sMessage, MSG_OK);
            FCLOSE(pFile);
        } else {
            strcat(sMessage,MSG_DATASET_NOT_FOUND);
        }
    }

    Lscpy(ARGR,sMessage);

    _style = _style_old;
}

void hostenv(int func) {
    int rc = 0,i=0;
    char *offset;
    char retbuf[320];
    byte *ect;
    byte *upt;
    void **cppl;

    memset(retbuf, '\0', sizeof(retbuf));

    if (isTSO()) cppl = entry_R13[6];
    else {
        Lscpy(ARGR,"failed, TSO required");
        return;
    }

    upt  = cppl[1];
    ect  = cppl[3];

    privilege(1);
    rc = systemCP(upt, ect, "CP QUERY CPLEVEL",16, retbuf,sizeof(retbuf));
    privilege(0);

    if (func==1) goto CPLEVEL;
    CPTYPE:
    if (strstr(retbuf, "HHC01600E")   != 0) Lscpy(ARGR, "Hercules");
    else if (strstr(retbuf, "VM/370") != 0) Lscpy(ARGR, "VM/370");
    else if (strstr(retbuf, "VM/ESA") != 0) Lscpy(ARGR, "VM/ESA");
    else if (strstr(retbuf, "VM/SP")  != 0) Lscpy(ARGR, "VM/SP");
    else if (strstr(retbuf, "VM")     != 0) Lscpy(ARGR, "VM");
    else Lscpy(ARGR, "UNKNOWN");
    return;

    CPLEVEL:
    if (strstr(retbuf, "HHC01600E") != 0) goto HercVersion;
    VMVersion:
    offset=strstr(retbuf, "VM/");
    if (offset==0) Lscpy(ARGR,retbuf);
    else {
        for (i = 0; offset[i] != '\0'; i++) {
            if (offset[i] == '\r' || offset[i] == '\n') {
                offset[i] = '\0';
                break;
            }
        }
        Lscpy(ARGR, offset);
    }
    return;

    HercVersion:
    privilege(1);
    rc = systemCP(upt, ect, "VERSION",7, retbuf,sizeof(retbuf));
    privilege(0);
    offset=strstr(retbuf, "Hercules");
    if (offset==0) Lscpy(ARGR,retbuf);
    else {
        for (i = 0; offset[i] != '\0'; i++) {
            if (offset[i] == 0x25) {
                offset[i] = '\0';
                break;
            }
        }
        Lscpy(ARGR, offset);
    }
    return;
}

void R_sysvar(int func)
{
    extern unsigned long long ullInstrCount;
    char *msg = "not yet implemented";

    if (ARGN != 1) {
        Lerror(ERR_INCORRECT_CALL,0);
    }

    LASCIIZ(*ARG1);
    get_s(1);
    Lupper(ARG1);

    if (strcmp((const char*)ARG1->pstr, "SYSUID") == 0) {
        Lscpy(ARGR,environment->SYSUID);
    } else if (strcmp((const char*)ARG1->pstr, "SYSPREF") == 0) {
        Lscpy(ARGR, environment->SYSPREF);
    } else if (strcmp((const char*)ARG1->pstr, "SYSENV") == 0) {
        if (!isTSO()) Lscpy(ARGR, "BATCH");
        else Lscpy(ARGR,environment->SYSENV);
    } else if (strcmp((const char*)ARG1->pstr, "SYSTSO") == 0) {
        Licpy(ARGR,isTSO());
    } else if (strcmp((const char*)ARG1->pstr, "SYSISPF") == 0) {
        Lscpy(ARGR, environment->SYSISPF);
    } else if (strcmp((const char*)ARG1->pstr, "SYSAUTH") == 0) {
        Licpy(ARGR, _testauth());
    } else if (strcmp((const char*)ARG1->pstr, "RXINSTRC") == 0) {
        Licpy(ARGR, ullInstrCount);
    } else if (strcmp((const char*)ARG1->pstr, "SYSHEAP") == 0) {
        Licpy(ARGR, __libc_heap_used);
    } else if (strcmp((const char*)ARG1->pstr, "SYSSTACK") == 0) {
        Licpy(ARGR, __libc_stack_used);
    } else if (strcmp((const char*)ARG1->pstr, "SYSCPLVL") == 0) {
        if (rac_check(FACILITY, SVC244, READ)) {
            hostenv(1);  // return argument set in hostenv()
        }  else {
            char *error_msg = "not authorized";
            Lscpy(ARGR, error_msg);
        }
    } else if (strcmp((const char*)ARG1->pstr, "SYSCP") == 0) {
        if (rac_check(FACILITY, SVC244, READ)) {
            hostenv(0);  // return argument set in hostenv()
        }  else {
            char *error_msg = "not authorized";
            Lscpy(ARGR, error_msg);
        }
    } else if (strcmp((const char*)ARG1->pstr, "SYSNODE") == 0) {
        if (rac_check(FACILITY, SVC244, READ)) {
            char netId[8 + 1];                // 8 + \0
            char *sNetId = &netId[0];
            privilege(1);
            RxNjeGetNetId(&sNetId);
            privilege(0);

            Lscpy(ARGR, sNetId);
        } else {
            char *error_msg = "not authorized";
            Lscpy(ARGR, error_msg);
        }
    } else if (strcmp((const char*)ARG1->pstr, "SYSRACF") == 0 ||
               strcmp((const char*)ARG1->pstr, "SYSRAKF") == 0) {
        if (rac_status()) {
            Lscpy(ARGR, "AVAILABLE");
        } else {
            Lscpy(ARGR, "NOT AVAILABLE");
        }
    } else if (strcmp((const char*)ARG1->pstr, "SYSTERMID") == 0) {

        RX_GTTERM_PARAMS paramsPtr;

        typedef struct tScreenSize {
            byte bRows;
            byte bCols;
        } PRIMARY_SCREEN_SIZE, ALTERNATE_SCREEN_SIZE;

        PRIMARY_SCREEN_SIZE primaryScreenSize;
        ALTERNATE_SCREEN_SIZE alternateScreenSize;

        char termid[8 + 1];

        paramsPtr.primadr   = (unsigned int *) &primaryScreenSize;
        paramsPtr.altadr    = (unsigned int *) &alternateScreenSize;
        *paramsPtr.altadr  |= 0x80000000;
        paramsPtr.attradr   = 0;
        paramsPtr.termidadr = (unsigned int *) &termid;

        bzero(termid, 9);

        gtterm(&paramsPtr);

        fprintf(stdout, "FOO> SYSTERMID=%s\n", termid);
        fprintf(stdout, "FOO> SYSWTERM=%d\n", alternateScreenSize.bCols);
        fprintf(stdout, "FOO> SYSLTERM=%d\n", alternateScreenSize.bRows);
        /*
        fssPrimaryCols      = primaryScreenSize.bCols;
        fssPrimaryRows      = primaryScreenSize.bRows;
        fssAlternateCols    = alternateScreenSize.bCols;
        fssAlternateRows    = alternateScreenSize.bRows;
        */

    } else {
        Lscpy(ARGR,msg);
    }
}

void R_terminal(int func) {

    int rows,cols;
    typedef struct tScreenSize {
        byte bRows;
        byte bCols;
    } SCREEN_SIZE;

    RX_GTTERM_PARAMS_PTR paramsPtr;
    SCREEN_SIZE ScreenSize;

    RX_SVC_PARAMS params;

    printf("Term 1\n");
    params.SVC = 94;
    params.R0  = (17 << 24);
    params.R1  = (unsigned)paramsPtr;
    printf("Term 2\n");

    call_rxsvc(&params);
    printf("Term 3\n");
    cols    = ScreenSize.bCols;
    rows    = ScreenSize.bRows;
    printf("Term 4\n");
    printf("ROWS/COLS %d %d\n",rows,cols);
}

void R_mvsvar(int func)
{
    char *msg = "not yet implemented";
    char chrtmp[16];
    char *tempoff;

    void ** psa;           // PSA     =>   0 / 0x00
    void ** cvt;           // FLCCVT  =>  16 / 0x10
    void ** smca;          // CVTSMCA => 196 / 0xC4
    void ** csd;           // CVT+660
    void ** smcasid;       // SMCASID =>  16 / 0x10
    short * cvt2;

    memset(chrtmp, '\0', sizeof(chrtmp));
    psa  = 0;
    cvt  = psa[4];         //  16
    smca = cvt[49];        // 196
    smcasid =  smca + 4;   //  16
    csd  = cvt[165];       // 660

    if (ARGN != 1) {
        Lerror(ERR_INCORRECT_CALL,0);
    }

    LASCIIZ(*ARG1);
    get_s(1);
    Lupper(ARG1);

    if (strcmp((const char *) ARG1->pstr, "SYSNAME") == 0) {
        Lscpy2(ARGR, (char *) (smcasid), 4);
    } else if (strcmp((const char *) ARG1->pstr, "SYSSMFID") == 0) {
        Lscpy2(ARGR, (char *) (smcasid), 4);
    } else if (strcmp((const char *) ARG1->pstr, "CPUS") == 0) {
        sprintf(&chrtmp[0], "%x", (int) csd[2]);
        tempoff = &chrtmp[0] + 4;
        sprintf(chrtmp, "%4s\n", tempoff);
        Lscpy2(ARGR, chrtmp, 4);
    } else if (strcmp((const char *) ARG1->pstr, "CPU") == 0) {
        sprintf(chrtmp, "%x", cvt[-2]);
        Lscpy(ARGR, chrtmp);
    } else if (strcmp((const char *) ARG1->pstr, "SYSOPSYS") == 0) {
        cvt2 = (short *) cvt;
        sprintf(chrtmp, "MVS %.*s.%.*s", 2, cvt2 - 2, 2, cvt2 - 1);
        Lscpy(ARGR, chrtmp);
    } else if (strcmp((const char *) ARG1->pstr, "SYSNJVER") == 0) {
        char version[21 + 1];             // 21 + \0
        char *sVersion = &version[0];
        RxNjeGetVersion(&sVersion);
        Lscpy(ARGR, sVersion);
        Lupper(ARGR);
    } else {
        Lscpy(ARGR, msg);
    }
}
/* ----- dropped way too slow!
void R_stemcopy(int func)
{
    BinTree *tree;
    PBinLeaf from, to, ptr ;
    Lstr tempKey, tempValue;

    LINITSTR(tempKey)
    LINITSTR(tempValue)

    Variable *varFrom, *varTo, *varTemp;

    if (ARGN!=2){
        Lerror(ERR_INCORRECT_CALL, 0);
    }

    // FROM
    Lupper(ARG1);
    LASCIIZ(*ARG1);

    // TO
    Lupper(ARG2);
    LASCIIZ(*ARG2);

    tree = _proc[_rx_proc].scope;

    // look up Source stem
    from = BinFind(tree, ARG1);
    if (!from) {
        printf("Invalid Stem %s\n", LSTR(*ARG1));
        Lerror(ERR_INCORRECT_CALL,0);
    }

    //  look up Target stem, must be available, later set it up
    to = BinFind(tree, ARG2);
    if (!to) {
        Lscpy(&tempKey,LSTR(*ARG2));
        Lcat(&tempKey,"$$STEMCOPY");
        setVariable(&tempKey,"");
        to = BinFind(tree, ARG2);
        if (!to) {
            printf("Target Stem missing %s\n", LSTR(*ARG2));
            Lerror(ERR_INCORRECT_CALL, 0);
        }
    }

    varFrom = (Variable *) from->value;
    varTo   = (Variable *) to->value;

    ptr = BinMin(varFrom->stem->parent);
    while (ptr != NULL) {
        Lstrcpy(&tempKey, &ptr->key);
        Lstrcpy(&tempValue, LEAFVAL(ptr));

        varTemp = (Variable *)MALLOC(sizeof(Variable),"Var");
        varTemp->value = tempValue;
        varTemp->exposed=((Variable *) ptr->value)->exposed;

//       BinAdd((BinTree *)varTo->stem, &tempKey, varTemp);

        ptr = BinSuccessor(ptr);
    }

    LFREESTR(tempKey)
    LFREESTR(tempValue)
}
*/


/* ---------------------------------------------------------------
 *  DIR( file )
 *    Exploiting Partitioned Data Set Directory Fields
 *      Part   I: http://www.naspa.net/magazine/1991/t9109019.txt
 *      Part  II: http://www.naspa.net/magazine/1991/t9110014.txt
 *      Part III: http://www.naspa.net/magazine/1991/t9111015.txt
 *    Using the System Status Index
 *                http://www.naspa.net/magazine/1991/t9104004.txt
 * ---------------------------------------------------------------
 */
void R_dir( const int func )
{
    int iErr;

    long   ii;

    FILE * fh;

    char   record[256];
    char   memberName[8 + 1];
    char   aliasName[8 + 1];
    char   ttr[6 + 1];
    char   version[5 + 1];
    char   creationDate[8 + 1];
    char   changeDate[8 + 1];
    char   changeTime[8 + 1];
    char   init[5 + 1];
    char   curr[5 + 1];
    char   mod[5 + 1];
    char   uid[8 + 1];

    unsigned char  *currentPosition;

    short  bytes;
    short  count;
    int    info_byte;
    short  numPointers;
    short  userDataLength;
    bool   isAlias;
    int    loadModuleSize;

    long   quit;
    short  l;
    char   sDSN[45];
    char   line[255];
    char   *sLine;
    char mode;
    int    pdsecount = 0;

    P_USER_DATA pUserData;

    if (ARGN < 1 || ARGN >2) {
        Lerror(ERR_INCORRECT_CALL,0);
    }

    must_exist(1);
    get_s(1)
    get_modev(2,mode,'D');

    LASCIIZ(*ARG1)

#ifndef __CROSS__
    Lupper(ARG1);
#endif

    bzero(sDSN, 45);

    _style = "//DSN:";

    // get the correct dsn for the input file
    iErr = getDatasetName(environment, (const char*)LSTR(*ARG1), sDSN);

    // open the pds directory
    fh = fopen (sDSN, "rb,klen=0,lrecl=256,blksize=256,recfm=u,force");

    if (fh != NULL) {
        // skip length field
        fread(&l, 1, 2, fh);

        quit = 0;

        while (fread(record, 1, 256, fh) == 256) {

            currentPosition = (unsigned char *) &(record[2]);
            bytes = ((short *) &(record[0]))[0];

            count = 2;
            while (count < bytes) {

                if (memcmp(currentPosition, endmark, 8) == 0) {
                    quit = 1;
                    break;
                }

                bzero(line, 255);
                sLine = line;

                bzero(memberName, 9);
                sprintf(memberName, "%.8s", currentPosition);
                {
                    // remove trailing blanks
                    long   jj = 7;
                    while (memberName[jj] == ' ') jj--;
                    memberName[++jj] = 0;
                }
                sLine += sprintf(sLine, "%-8s", memberName);
                    currentPosition += 8;   // skip current member name

                    bzero(ttr, 7);
                    sprintf(ttr, "%.2X%.2X%.2X", currentPosition[0], currentPosition[1], currentPosition[2]);
                    sLine += sprintf(sLine, "   %-6s", ttr);
                    currentPosition += 3;   // skip ttr

                    info_byte = (int) (*currentPosition);
                    currentPosition += 1;   // skip info / stats byte

                numPointers    = (info_byte & NPTR_MASK);
                    userDataLength = (info_byte & UDL_MASK) * 2;

                    // no load lib
                if (numPointers == 0 && userDataLength > 0) {
                    int year = 0;
                    int day = 0;
                    char *datePtr;

                    pUserData = (P_USER_DATA) currentPosition;
                    if (mode != 'M') {
                        bzero(version, 6);
                        sprintf(version, "%.2d.%.2d", pUserData->vlvl, pUserData->mlvl);
                        sLine += sprintf(sLine, " %-5s", version);
                        bzero(creationDate, 9);
                        datePtr = (char *) &creationDate;
                        year = getYear(pUserData->credt[0], pUserData->credt[1]);
                        day = getDay(pUserData->credt[2], pUserData->credt[3]);
                        julian2gregorian(year, day, &datePtr);
                        sLine += sprintf(sLine, " %-8s", creationDate);

                        bzero(changeDate, 9);
                        datePtr = (char *) &changeDate;
                        year = getYear(pUserData->chgdt[0], pUserData->chgdt[1]);
                        day = getDay(pUserData->chgdt[2], pUserData->chgdt[3]);
                        julian2gregorian(year, day, &datePtr);
                        sLine += sprintf(sLine, " %-8s", changeDate);

                        bzero(changeTime, 9);
                        sprintf(changeTime, "%.2x:%.2x:%.2x", (int) pUserData->chgtm[0], (int) pUserData->chgtm[1],
                                (int) pUserData->chgss);
                        sLine += sprintf(sLine, " %-8s", changeTime);

                        bzero(init, 6);
                        sprintf(init, "%5d", pUserData->init);
                        sLine += sprintf(sLine, " %-5s", init);

                        bzero(curr, 6);
                        sprintf(curr, "%5d", pUserData->curr);
                        sLine += sprintf(sLine, " %-5s", curr);

                        bzero(mod, 6);
                        sprintf(mod, "%5d", pUserData->mod);
                        sLine += sprintf(sLine, " %-5s", mod);

                        bzero(uid, 9);
                        sprintf(uid, "%-.8s", pUserData->uid);
                        sLine += sprintf(sLine, " %-8s", uid);
                    }
                } else {
                    isAlias = (info_byte & ALIAS_MASK);

                    loadModuleSize = ((byte) *(currentPosition + 0xA)) << 16 |
                                     ((byte) *(currentPosition + 0xB)) << 8 |
                                     ((byte) *(currentPosition + 0xC));

                    sLine += sprintf(sLine, " %.6x", loadModuleSize);

                    if (isAlias) {
                        bzero(aliasName, 9);
                        sprintf(aliasName, "%.8s", currentPosition + 0x18);
                        {
                            // remove trailing blanks
                            long jj = 7;
                            while (aliasName[jj] == ' ') jj--;
                            aliasName[++jj] = 0;
                        }
                        sLine += sprintf(sLine, " %.8s", aliasName);
                    }
                }

                if (pdsecount == maxdirent) {
                    quit = 1;
                    break;
                } else {
                    char stemName[13]; // DIRENTRY (8) + . (1) + MAXDIRENTRY=3000 (4)
                    char varName[32];

                    bzero(stemName, 13);
                    bzero(varName, 32);

                    sprintf(stemName, "DIRENTRY.%d", ++pdsecount);

                    sprintf(varName, "%s.NAME", stemName);
                    setVariable(varName, memberName);
                    if (mode=='D') {
                        sprintf(varName, "%s.TTR", stemName);
                        setVariable(varName, ttr);

                        if ((((info_byte & 0x60) >> 5) == 0) && userDataLength > 0) {
                            sprintf(varName, "%s.CDATE", stemName);
                            setVariable(varName, creationDate);

                            sprintf(varName, "%s.UDATE", stemName);
                            setVariable(varName, changeDate);

                            sprintf(varName, "%s.UTIME", stemName);
                            setVariable(varName, changeTime);

                            sprintf(varName, "%s.INIT", stemName);
                            setVariable(varName, init);

                            sprintf(varName, "%s.SIZE", stemName);
                            setVariable(varName, curr);

                            sprintf(varName, "%s.MOD", stemName);
                            setVariable(varName, mod);

                            sprintf(varName, "%s.UID", stemName);
                            setVariable(varName, uid);
                        }
                    }
                    if (mode!='M') {
                        sprintf(varName, "%s.LINE", stemName);
                        setVariable(varName, line);
                    }
                }

                currentPosition += userDataLength;

                count += (8 + 4 + userDataLength);
            }

            if (quit) break;

            fread(&l, 1, 2, fh); /* Skip U length */
        }

        fclose(fh);
        _style = "//DDN:";
        setIntegerVariable("DIRENTRY.0", pdsecount);
        Licpy(ARGR,0);
    }  else Licpy(ARGR,8);
}

void R_locate (const int func )
{
    int rc, info_byte, stop=0, jj;
    short l, bytes, count, userDataLength;
    FILE *fh;
    char record[256];
    char memberName[8 + 1];
    unsigned char *currentPosition;

    if (ARGN >3 && ARGN<2) {
        Lerror(ERR_INCORRECT_CALL, 0);
    }

    get_s(1)
    get_s(2)

    LASCIIZ(*ARG1)
    LASCIIZ(*ARG2)
    Lupper(ARG1);
    Lupper(ARG2);

    _style = "//DSN:";
    if (ARGN==3) {
        get_s(3)
        Lupper(ARG3);
        if (strcmp(LSTR(*ARG3), "FILE") == 0) _style = "//DDN:";
    }

#ifndef __CROSS__
    Lupper(ARG1);
#endif
 // for performance reasons we expect always fully qualified DSNs
    fh = fopen((const char *)LSTR(*ARG1), "rb,klen=0,lrecl=256,blksize=256,recfm=u,force");
    rc = 12;
 //   printf("Open '%s' %d %s %d\n",LSTR(*ARG1),fh,_style,ARGN);
    if (fh == NULL) goto notopen;
 // skip length field
    fread(&l, 1, 2, fh);
    rc = 8;    // default Member not found
    stop=0;    // default for ending directory loop
    while (fread(record, 1, 256, fh) == 256) {
        currentPosition = (unsigned char *) &(record[2]);
        bytes = ((short *) &(record[0]))[0];
        count = 2;
        while (count < bytes) {
           if (memcmp(currentPosition, endmark, 8) == 0){
                stop=1;
                break;
            }  // end of directory reached
            memcpy(memberName,currentPosition,8);
            jj = 7;
            while (memberName[jj] == ' ') jj--;
            memberName[++jj] = 0;
            if (strcmp(LSTR(*ARG2), memberName) == 0) {
                stop=1;
                rc = 0;
                break;
            } // member found, end search
            currentPosition += 11;   // skip current member name + ttr
            info_byte = (int) (*currentPosition);
            currentPosition += 1;
            userDataLength = (info_byte & UDL_MASK) * 2;
            currentPosition += userDataLength;
            count += (8 + 4 + userDataLength);
        }
        if (stop==1) break;
        fread(&l, 1, 2, fh); /* Skip U length */
    }
    fclose(fh);
    notopen:
    _style = "//DDN:";
    Licpy(ARGR, rc);
}

/* -------------------------------------------------------------------------------------
 * return integer value, REAL numbers will converted to integer, STRING parms lead to error
 * -------------------------------------------------------------------------------------
 */
void R_int( const int func ) {

    if (ARGN != 1) Lerror(ERR_INCORRECT_CALL, 0);
    if (LTYPE(*ARG1) == LINTEGER_TY) Licpy(ARGR, LINT(*ARG1));
    else if (LTYPE(*ARG1) == LREAL_TY) Licpy(ARGR, LREAL(*ARG1));
    else {
        L2STR(ARG1);
        if (_Lisnum(ARG1)==LSTRING_TY) Lfailure("Invalid Number: ",LSTR(*ARG1),"","","");
        LINT(*ARGR) = (long) lLastScannedNumber;
        LTYPE(*ARGR) = LINTEGER_TY;
        LLEN(*ARGR) = sizeof(long);
    }
}

/* -------------------------------------------------------------------------------------
 * Fast variant of DATATYPE
 * -------------------------------------------------------------------------------------
 */
void R_type( const int func ) {

    int ta;

    if (ARGN != 1) Lerror(ERR_INCORRECT_CALL, 0);
    if (LTYPE(*ARG1) == LINTEGER_TY) Lscpy(ARGR, "INTEGER");
    else if (LTYPE(*ARG1) == LREAL_TY) Lscpy(ARGR, "REAL");
    else {
        L2STR(ARG1);
        switch (_Lisnum(ARG1)) {
            case LINTEGER_TY:
                Lscpy(ARGR, "INTEGER");
                break;
            case LREAL_TY:
                Lscpy(ARGR, "REAL");
                break;
            case LSTRING_TY:
                Lscpy(ARGR, "STRING");
                break;
        }
    }
}

/* -------------------------------------------------------------------------------------
 * Encrypt String
 * -------------------------------------------------------------------------------------
 */
void R_crypt(int func) {
    int rounds=7;
    // string to encrypt and password must exist
    must_exist(1);
    must_exist(2);
    get_oi0(3,rounds);       /* drop rounds parameter, it might decrease security */
    if (rounds==0) rounds=7;  /* maximum slots */
    Lcryptall(ARGR, ARG1, ARG2,rounds,0);  // mode =0  encode
}

/* -------------------------------------------------------------------------------------
 * Decrypt String
 * -------------------------------------------------------------------------------------
 */
void R_decrypt(int func) {
    int rounds=1;
    // string to encrypt and password must exist
    must_exist(1);
    must_exist(2);
    Lcryptall(ARGR, ARG1, ARG2,rounds,1); // mode =1  decode
}

/* -------------------------------------------------------------------------------------
 * Rotate String (registered stub)
 * -------------------------------------------------------------------------------------
 */
void R_rotate(int func) {
    int start, slen;
    must_exist(1);
    must_exist(2);
    get_oi(2,start);
    get_oi0(3,slen);
    _rotate(ARGR,ARG1,start,slen);
}

/* -------------------------------------------------------------------------------------
 * RHASH (registered stub)
 * -------------------------------------------------------------------------------------
 */
>>>>>>> dc8e002663f91d8eef0f45b6b760c3168e050fe9
void R_rhash(int func) {
    int     slots=0;

    must_exist(1);
    get_oi0(2,slots);       /* is there a max slot given? */

    Lhash(ARGR,ARG1,slots);
}

// -------------------------------------------------------------------------------------
// Remove DSN
// -------------------------------------------------------------------------------------
void R_removedsn(int func)
{
    char sFileNameÝ55¨;
    int remrc=-2, iErr=0,dbg=0;
    char* _style_old = _style;

    memset(sFileName,0,55);
    if (ARGN !=1) Lerror(ERR_INCORRECT_CALL,0);
    LASCIIZ(*ARG1)
#ifndef __CROSS__
    Lupper(ARG1);
#endif
    get_s(1)
    _style = "//DSN:";
    iErr = getDatasetName(environment, (const char *) LSTR(*ARG1), sFileName);
    // no errors occurred so far, perform the remove
    if (iErr == 0) remrc = remove(sFileName);
    else remrc=iErr;

    if (dbg==1) {
        printf("Remove %s\n",sFileName);
        printf("   RC  %i\n",remrc);
    }

    Licpy(ARGR,remrc);
    _style = _style_old;
}
// -------------------------------------------------------------------------------------
// Rename DSN-old,DSN-new
// -------------------------------------------------------------------------------------
void R_renamedsn(int func)
{
    char sFileNameOldÝ55¨;
    Lstr oldDSN, oldMember;
    char sFileNameNewÝ55¨;
    Lstr newDSN, newMember;
    char sFunctionCodeÝ3¨;
    int renrc=-9, iErr=0, p=0, dbg=0;
    char* _style_old = _style;

    if (ARGN !=2) Lerror(ERR_INCORRECT_CALL,0);

    memset(sFileNameOld,0,55);
    memset(sFileNameNew,0,55);

    LASCIIZ(*ARG1)
    LASCIIZ(*ARG2)
    get_s(1)
    get_s(2)

#ifndef __CROSS__
    Lupper(ARG1);
    Lupper(ARG2);
#endif
// * ---------------------------------------------------------------------------------------
// * Split DSN and Member
// * ---------------------------------------------------------------------------------------
    splitDSN(&oldDSN, &oldMember, ARG1);
    splitDSN(&newDSN, &newMember, ARG2);
// * ---------------------------------------------------------------------------------------
// * Auto complete DSNs
// * ---------------------------------------------------------------------------------------
    _style = "//DSN:";
    iErr = getDatasetName(environment, (const char *) LSTR(oldDSN), sFileNameOld);
    if (iErr == 0) {
        iErr = getDatasetName(environment, (const char *) LSTR(newDSN), sFileNameNew);
        if (iErr != 0) renrc=-2;
    } else renrc=-2;
    if (iErr != 0) goto leave;
//* Add Member Names if there are any
    if (LLEN(oldMember)>0) {
        strcat(sFileNameOld, "(");
        strcat(sFileNameOld, (const char *) LSTR(oldMember));
        strcat(sFileNameOld, ")");
    }
    if (LLEN(newMember)>0) {
        strcat(sFileNameNew, "(");
        strcat(sFileNameNew, (const char *) LSTR(newMember));
        strcat(sFileNameNew, ")");
    }
// * ---------------------------------------------------------------------------------------
// * Test certain RENAME some scenarios
// * ---------------------------------------------------------------------------------------
    if (LLEN(oldMember)==0 && LLEN(newMember)!=0 || LLEN(oldMember)!=0 && LLEN(newMember)==0) goto incomplete;
    if (Lstrcmp(&oldDSN,&newDSN)==0 ){
        if (LLEN(oldMember)==0 && LLEN(newMember)==0) goto STequal;
        if (LLEN(oldMember)>0 && LLEN(newMember)>0) {
            if (strcmp((const char *) LSTR(oldMember),(const char *) LSTR(newMember))==0) goto STequal;
            goto doRename;  // perform Member Rename
        }
    }
    if (Lstrcmp(&oldDSN,&newDSN)!=0 ) {
        if (LLEN(oldMember) > 0 && LLEN(newMember) > 0) goto invalren;
        else if (LLEN(oldMember) == 0 && LLEN(newMember) == 0) goto doRename;
    }
    goto doRename;  // no match with special secenarious, just try the rename/*
// * ---------------------------------------------------------------------------------------
// * Incomplete Member definition in either from or to DSN
// * ---------------------------------------------------------------------------------------
    incomplete:
    if (dbg==1) printf("incomplete Member definition in Rename\n");
    renrc=-3;
    goto leave;
// * ---------------------------------------------------------------------------------------
// * From / To DSNs are equal, no Rename necessary
// * ---------------------------------------------------------------------------------------
    STequal:
    if (dbg==1) printf("Source and Target DSN are equal\n");
    renrc=-4;
    goto leave;
//  * ---------------------------------------------------------------------------------------
//  * DSN Rename and Member Rename at the same time are not allowed
//  * ---------------------------------------------------------------------------------------
    invalren:
    if (dbg==1) printf("Invalid Rename of DSN and Member at the same time\n");
    renrc = -5;
    goto leave;
// * ---------------------------------------------------------------------------------------
// * Perform the Rename
// * ---------------------------------------------------------------------------------------
    doRename:
#ifndef __CROSS__
    renrc = rename(sFileNameOld,sFileNameNew);
#else
    renrc=0;
#endif
// * ---------------------------------------------------------------------------------------
// * Clean up and exit
// * ---------------------------------------------------------------------------------------
    leave:
    if (dbg==1) {
        printf("Rename from %s\n",sFileNameOld);
        printf("         To %s\n",sFileNameNew);
        printf("         RC %i\n",renrc);
    }
    LFREESTR(oldDSN);
    LFREESTR(oldMember);
    LFREESTR(newDSN);
    LFREESTR(newMember);
    Licpy(ARGR,renrc);
    L2int(ARGR);
    _style = _style_old;
}
// -------------------------------------------------------------------------------------
// DYNFREE  ddname
// -------------------------------------------------------------------------------------
void R_free(int func)
{
    int iErr=0,dbg=0;
    __dyn_t dyn_parms;

    if (ARGN !=1) Lerror(ERR_INCORRECT_CALL,0);

    LASCIIZ(*ARG1)
    get_s(1)

#ifndef __CROSS__
    Lupper(ARG1);
#endif

    dyninit(&dyn_parms);
    dyn_parms.__ddname = (char *) LSTR(*ARG1);

    iErr = dynfree(&dyn_parms);
    if (dbg==1) {
        printf("FREE DD %s\n",LSTR(*ARG1));
        printf("     RC %i\n",iErr);
    }

    Licpy(ARGR, iErr);
}
// -------------------------------------------------------------------------------------
// DYNALLOC ddname DSN SHR
// -------------------------------------------------------------------------------------
void R_allocate(int func) {
    int iErr = 0, dbg=0;
    char *_style_old = _style;
    char sFileNameÝ55¨;
    Lstr DSN, Member;
    __dyn_t dyn_parms;
    if (ARGN !=2) Lerror(ERR_INCORRECT_CALL, 0);

    LASCIIZ(*ARG1)
    LASCIIZ(*ARG2)
    get_s(1)
    get_s(2)
#ifndef __CROSS__
    Lupper(ARG1);
    Lupper(ARG2);
#endif
    _style = "//DSN:";    // Complete DSN if necessary
    splitDSN(&DSN, &Member, ARG2);
    iErr = getDatasetName(environment, (const char *) LSTR(DSN), sFileName);
    if (iErr == 0) {
        dyninit(&dyn_parms);
        dyn_parms.__ddname = (char *) LSTR(*ARG1);
        // free DDNAME, just in case it's allocated
        iErr = dynfree(&dyn_parms);

        dyn_parms.__dsname = (char *) sFileName;
        if (LLEN(Member)>0) dyn_parms.__member = (char *) LSTR(Member);
        dyn_parms.__status = __DISP_SHR;

        iErr = dynalloc(&dyn_parms);
        if (dbg==1) {
            printf("ALLOC DD %s\n",LSTR(*ARG1));
            printf("     DSN %s\n",sFileName);
            if (LLEN(Member)>0)  printf("  Member %s\n",LSTR(Member));
            printf("      RC %i\n",iErr);
        }
    }
    Licpy(ARGR,iErr);
    LFREESTR(DSN);
    LFREESTR(Member);

    _style = _style_old;
}
// -------------------------------------------------------------------------------------
// CREATE new Dataset
// -------------------------------------------------------------------------------------
void R_create(int func) {
    int iErr = 0,dbg=0;
    char sFileNameÝ55¨;
    char sFileDCBÝ128¨;
    char *_style_old = _style;
    FILE *fk ; // file handle

    if (ARGN !=2) Lerror(ERR_INCORRECT_CALL, 0);

    LASCIIZ(*ARG1)
    LASCIIZ(*ARG2)
    get_s(1)
    get_s(2)

#ifndef __CROSS__
    Lupper(ARG1);
    Lupper(ARG2);
#endif
    memset(sFileDCB,0,80);
    strcat(sFileDCB, "WB, ");
    strcat(sFileDCB, (const char *) LSTR(*ARG2));
    _style = "//DSN:";    // Complete DSN if necessary
    iErr = getDatasetName(environment, (const char *) LSTR(*ARG1), sFileName);
    if (iErr == 0) {
        fk=FOPEN((char*) sFileName,"RB");
        if (fk!=NULL) { // File already defined, error
            FCLOSE(fk);
            if (dbg==1) printf("DSN already catalogued %s\n", sFileName);
            iErr = -2;
        }
    }
    if (iErr == 0) {
        fk=FOPEN((char*) sFileName,sFileDCB);
        if (fk!=NULL) { // File sucessfully created
            FCLOSE(fk);
            if (dbg==1) printf("DSN created successfully %s\n", sFileName);
            iErr = 0;
        } else {
            if (dbg==1) printf("DSN cannot be created %s\n", sFileName);
            iErr = -1;
        }
    }
    if (dbg==1) {
        printf("CREATE     %s\n",sFileName);
        printf("  DCB etc. %s\n",sFileDCB);
        printf("       RC  %i\n",iErr);
    }

    Licpy(ARGR,iErr);
    _style = _style_old;
}
// -------------------------------------------------------------------------------------
// EXISTS does Dataset exist
// -------------------------------------------------------------------------------------
void R_exists(int func) {
    int iErr = 0;
    char sFileNameÝ55¨;
    char *_style_old = _style;
    FILE *fk; // file handle

    if (ARGN != 1) Lerror(ERR_INCORRECT_CALL, 0);

    LASCIIZ(*ARG1)
    get_s(1)
#ifndef __CROSS__
    Lupper(ARG1);
#endif
    _style = "//DSN:";    // Complete DSN if necessary
    iErr = getDatasetName(environment, (const char *) LSTR(*ARG1), sFileName);
    if (iErr == 0) {
        fk = FOPEN((char *) sFileName, "RB");
        if (fk != NULL) { // File already defined, error
            FCLOSE(fk);
            iErr = 1;
        } else iErr=0;
    }
    Licpy(ARGR,iErr);
    _style = _style_old;
}



#ifdef __DEBUG__
void R_magic(int func)
{
    void *pointer;
    long decAddr;
    int  count;
    char magicstrÝ64¨;

    char option='F';

    if (ARGN>1)
        Lerror(ERR_INCORRECT_CALL,0);
    if (exist(1)) {
        L2STR(ARG1);
        option = l2uÝ(byte)LSTR(*ARG1)Ý0¨¨;
    }

    option = l2uÝ(byte)option¨;

    switch (option) {
        case 'F':
            pointer = mem_first();
            decAddr = (long) pointer;
            sprintf(magicstr,"%ld", decAddr);
            break;
        case 'L':
            pointer = mem_last();
            decAddr = (long) pointer;
            sprintf(magicstr,"%ld", decAddr);
            break;
        case 'C':
            count = mem_count();
            sprintf(magicstr,"%d", count);
            break;
        default:
            sprintf(magicstr,"%s", "ERROR");
    }

    Lscpy(ARGR,magicstr);
}

void R_dummy(int func)
{
    void *nextPtr = 0x00;

#ifdef __CROSS__

    BinTree tree = _procÝ_rx_proc¨.scopeÝ0¨;
    BinPrint(tree.parent, NULL);
    /*
    do {
        printf("FOO> %s\n", getNextVar(&nextPtr));
    }
    while (nextPtr != NULL);
     */
#endif

}
#endif

int RxMvsInitialize()
{
    RX_INIT_PARAMS_PTR init_parameter;
    RX_TSO_PARAMS_PTR  tso_parameter;
    RX_ENVIRONMENT_BLK_PTR env_block;

    void ** pEnvBlock;

    int      rc     = 0;

#ifdef __DEBUG__
    if (entry_R13 != 0) {
        printf("DBG> SA at %08X\n", (unsigned) entry_R13);
    }
#endif

    init_parameter   = malloc(sizeof(RX_INIT_PARAMS));
    memset(init_parameter, 0, sizeof(RX_INIT_PARAMS));

    environment      = malloc(sizeof(RX_ENVIRONMENT_CTX));
    memset(environment, 0, sizeof(RX_ENVIRONMENT_CTX));

    init_parameter->rxctxadr = (unsigned *)environment;

    rc = call_rxinit(init_parameter);

    if ((environment->flags3 & _STDIN) == _STDIN) {
        reopen(_STDIN);
    }
    if ((environment->flags3 & _STDOUT) == _STDOUT) {
        reopen(_STDOUT);
    }
    if ((environment->flags3 & _STDERR) == _STDERR) {
        reopen(_STDERR);
    }

    free(init_parameter);

#ifdef __DEBUG__
    printf("DBG> ENVIRONMENT CONTEXT AT %08X\n", (unsigned)environment);
    DumpHex((unsigned char*)environment, sizeof(RX_ENVIRONMENT_CTX) - (64*4));
    printf("\n");
#endif

    env_block = malloc(sizeof(RX_ENVIRONMENT_BLK));
    memset((env_block), 0, sizeof(RX_ENVIRONMENT_BLK));
    memcpy(env_block->envblock_id, "ENVBLOCK", 8);
    memcpy(env_block->envblock_version, "0100", 4);
    env_block->envblock_length = 320;

    if (isTSO()) {
        setEnvBlock(env_block);
    }

    return rc;
}

int reopen(int fp) {

    int new_fp, rc = 0;
    char* _style_old = _style;

#ifdef JCC
    _style = "//DDN:";
    switch(fp) {
        case 0x01:
            if (stdin != NULL) {
              fclose(stdin);
            }

            new_fp = _open("STDIN", O_TEXT | O_RDONLY);
            rc = _dup2(new_fp, 0);
            _close(new_fp);

            stdin = fdopen(0,"rt");

            break;
        case 0X02:
            if (stdout != NULL) {
              fclose(stdout);
            }

            new_fp = _open("STDOUT", O_TEXT | O_WRONLY);
            rc = _dup2(new_fp, 1);
            _close(new_fp);

            stdout = fdopen(1,"at");

            break;
        case 0x04:
            if (stderr != NULL) {
              fclose(stderr);
            }

            new_fp = _open("STDERR", O_TEXT | O_WRONLY);
            rc = _dup2(new_fp, 2);
            _close(new_fp);

            stderr = fdopen(2, "at");

            break;
        default:
            rc = ERR_INITIALIZATION;
            break;
    }
#endif
    _style = _style_old;

    return 0;
}

void RxMvsRegFunctions()
{
    RxRacRegFunctions();
    RxTcpRegFunctions();

    /* MVS specific functions */
    RxRegFunction("ENCRYPT",    R_crypt,        0);
    RxRegFunction("DECRYPT",    R_decrypt,      0);
    RxRegFunction("FREE",       R_free,         0);
    RxRegFunction("ALLOCATE",   R_allocate,     0);
    RxRegFunction("CREATE",     R_create,       0);
    RxRegFunction("EXISTS",     R_exists,       0);
    RxRegFunction("RENAME",     R_renamedsn,    0);
    RxRegFunction("REMOVE",     R_removedsn,    0);
    RxRegFunction("DUMPIT",     R_dumpIt,       0);
    RxRegFunction("LISTIT",     R_listIt,       0);
    RxRegFunction("WAIT",       R_wait,         0);
    RxRegFunction("WTO",        R_wto ,         0);
    RxRegFunction("ABEND",      R_abend ,       0);
    RxRegFunction("USERID",     R_userid,       0);
    RxRegFunction("LISTDSI",    R_listdsi,      0);
    RxRegFunction("ROTATE",     R_rotate,       0);
    RxRegFunction("RHASH",      R_rhash,        0);
    RxRegFunction("SYSDSN",     R_sysdsn,       0);
    RxRegFunction("SYSVAR",     R_sysvar,       0);
    RxRegFunction("UPPER",      R_upper,  0);
    RxRegFunction("JOIN",      R_join,  0);
    RxRegFunction("SPLIT",      R_split,  0);
    RxRegFunction("LOWER",      R_lower,  0);
    RxRegFunction("LASTWORD",   R_lastword,  0);
    RxRegFunction("VLIST",      R_vlist,        0);
    RxRegFunction("BLDL",       R_bldl,         0);
    RxRegFunction("STEMCOPY",   R_stemcopy,     0);
    RxRegFunction("DIR",        R_dir,          0);
    RxRegFunction("CPUUSAGE",   R_cputime,          0);
#ifdef __DEBUG__
    RxRegFunction("MAGIC",      R_magic,        0);
    RxRegFunction("DUMMY",      R_dummy,        0);
    RxRegFunction("VXGET",      R_vxget,        0);
    RxRegFunction("VXPUT",      R_vxput,        0);
    RxRegFunction("CATCHIT",    R_catchIt,      0);
#endif
}

int isTSO() {
    int ret = 0;

    if ((environment->flags2 & _TSOFG) == _TSOFG ||
        (environment->flags2 & _TSOBG) == _TSOBG) {
        ret = 1;
    }

    return ret;
}

int isISPF() {
    int ret = 0;

    if ((environment->flags2 & _ISPF) == _ISPF) {
        ret = 1;
    }

    return ret;
}

void parseArgs(char **array, char *str)
{
    int i = 0;
    char *p = strtok (str, " ");
    while (p != NULL)
    {
        arrayÝi++¨ = p;
        p = strtok (NULL, " ");
    }
}

void parseDCB(FILE *pFile)
{
    unsigned char *flags;
    unsigned char  sDsnÝ45¨;
    unsigned char  sDdnÝ9¨;
    unsigned char  sMemberÝ9¨;
    unsigned char  sSerialÝ7¨;
    unsigned char  sLreclÝ6¨;
    unsigned char  sBlkSizeÝ6¨;

    flags = malloc(11);
    __get_ddndsnmemb(fileno(pFile), (char *)sDdn, (char *)sDsn, (char *)sMember, (char *)sSerial, flags);

    /* DSN */
    if (sDsnÝ0¨ != '\0')
        setVariable("SYSDSNAME", (char *)sDsn);

    /* DDN */
    if (sDdnÝ0¨ != '\0')
        setVariable("SYSDDNAME", (char *)sDdn);

    /* MEMBER */
    if (sMemberÝ0¨ != '\0')
        setVariable("SYSMEMBER", (char *)sMember);

    /* VOLSER */
    if (sSerialÝ0¨ != '\0')
        setVariable("SYSVOLUME", (char *)sSerial);

    /* DSORG */
    if(flagsÝ4¨ == 0x40)
        setVariable("SYSDSORG", "PS");
    else if (flagsÝ4¨ == 0x02)
        setVariable("SYSDSORG", "PO");
    else
        setVariable("SYSDSORG", "???");

    /* RECFM */
    if(flagsÝ6¨ == 0x40)
        setVariable("SYSRECFM", "V");
    else if(flagsÝ6¨ == 0x50)
        setVariable("SYSRECFM", "VB");
    else if(flagsÝ6¨ == 0x54)
        setVariable("SYSRECFM", "VBA");
    else if(flagsÝ6¨ == 0x80)
        setVariable("SYSRECFM", "F");
    else if(flagsÝ6¨ == 0x90)
        setVariable("SYSRECFM", "FB");
    else if(flagsÝ6¨ == 0xC0)
        setVariable("SYSRECFM", "U");
    else
        setVariable("SYSRECFM", "??????");

    /* BLKSIZE */
    sprintf((char *)sBlkSize, "%d", flagsÝ8¨ | flagsÝ7¨ << 8);
    setVariable("SYSBLKSIZE", (char *)sBlkSize);

    /* LRECL */
    sprintf((char *)sLrecl, "%d", flagsÝ10¨ | flagsÝ9¨ << 8);
    setVariable("SYSLRECL", (char *)sLrecl);

    free(flags);
}

void *
_getEctEnvBk()
{
    void ** psa;           // PAS      =>   0 / 0x00
    void ** ascb;          // PSAAOLD  => 548 / 0x224
    void ** asxb;          // ASCBASXB => 108 / 0x6C
    void ** lwa;           // ASXBLWA  =>  20 / 0x14
    void ** ect;           // LWAPECT  =>  32 / 0x20
    void ** ectenvbk;      // ECTENVBK =>  48 / 0x30

    if (isTSO()) {
        psa  = 0;
        ascb = psaÝ137¨;
        asxb = ascbÝ27¨;
        lwa  = asxbÝ5¨;
        ect  = lwaÝ8¨;

        // TODO use cast to BYTE and + 48
        ectenvbk = ect + 12;   // 12 * 4 = 48

    } else {
        ectenvbk = NULL;
    }

    return ectenvbk;
}

void *
getEnvBlock()
{
    void **ectenvbk;
    void  *envblock;

    ectenvbk = _getEctEnvBk();

    if (ectenvbk != NULL) {
        envblock = *ectenvbk;
    } else {
        envblock = NULL;
    }

    return envblock;
}

void
setEnvBlock(void *envblk)
{
    void ** ectenvbk;

    ectenvbk  = _getEctEnvBk();

    if (ectenvbk != NULL) {
        *ectenvbk = envblk;
    }
}

void
getVariable(char *sName, PLstr plsValue)
{
    Lstr lsScope,lsName;

    LINITSTR(lsScope)
    LINITSTR(lsName)

    Lfx(&lsScope,sizeof(dword));
    Lfx(&lsName, strlen(sName));

    Licpy(&lsScope,_rx_proc);
    Lscpy(&lsName, sName);

    RxPoolGet(&lsScope, &lsName, plsValue);

    LASCIIZ(*plsValue)

    LFREESTR(lsScope)
    LFREESTR(lsName)
}

char *
getStemVariable(char *sName)
{
    char  sValueÝ4097¨;
    Lstr lsScope,lsName,lsValue;

    LINITSTR(lsScope)
    LINITSTR(lsName)
    LINITSTR(lsValue)

    Lfx(&lsScope,sizeof(dword));
    Lfx(&lsName, strlen(sName));

    Licpy(&lsScope,_rx_proc);
    Lscpy(&lsName, sName);

    RxPoolGet(&lsScope, &lsName, &lsValue);

    LASCIIZ(lsValue)

    if(LTYPE(lsValue)==1) {
        sprintf(sValue,"%d",LINT(lsValue));
    }
    if(LTYPE(lsValue)==2) {
        sprintf(sValue,"%f",LREAL(lsValue));
    }
    if(LTYPE(lsValue)==0) {
        memset(sValue,0,sizeof(sValue));
        strncpy(sValue,LSTR(lsValue),LLEN(lsValue));
    }

    LFREESTR(lsScope)
    LFREESTR(lsName)
    LFREESTR(lsValue)

    return (char *)sValueÝ0¨;
}

int
getIntegerVariable(char *sName) {
    char sValueÝ19¨;
    PLstr plsValue;
    LPMALLOC(plsValue)
    getVariable(sName, plsValue);

    if(LTYPE(*plsValue)==1) {
        sprintf(sValue,"%d",(int)LINT(*plsValue));
    } else if (LTYPE(*plsValue)==0) {
        memset(sValue,0,sizeof(sValue));
        strncpy(sValue,(const char*)LSTR(*plsValue),LLEN(*plsValue));
    } else {
        sprintf(sValue,"%d",0);
    }

    return (atoi(sValue));
}

void
setVariable(char *sName, char *sValue)
{
    Lstr lsScope,lsName,lsValue;

    LINITSTR(lsScope)
    LINITSTR(lsName)
    LINITSTR(lsValue)

    Lfx(&lsScope,sizeof(dword));
    Lfx(&lsName, strlen(sName));
    Lfx(&lsValue, strlen(sValue));

    Licpy(&lsScope,_rx_proc);
    Lscpy(&lsName, sName);
    Lscpy(&lsValue, sValue);
    RxPoolSet(&lsScope, &lsName, &lsValue);

    LFREESTR(lsScope)
    LFREESTR(lsName)
    LFREESTR(lsValue)
}

void
setVariable2(char *sName, char *sValue, int lValue)
{
    Lstr lsScope,lsName,lsValue;

    LINITSTR(lsScope)
    LINITSTR(lsName)
    LINITSTR(lsValue)

    Lfx(&lsScope,sizeof(dword));
    Lfx(&lsName, strlen(sName));
    Lfx(&lsValue, lValue);

    Licpy(&lsScope,_rx_proc);
    Lscpy(&lsName, sName);
    Lscpy2(&lsValue, sValue, lValue);

    RxPoolSet(&lsScope, &lsName, &lsValue);

    LFREESTR(lsScope)
    LFREESTR(lsName)
    LFREESTR(lsValue)
}

void
setIntegerVariable(char *sName, int iValue)
{
    char sValueÝ19¨;

    sprintf(sValue,"%d",iValue);
    setVariable(sName,sValue);
}

int
GetClistVar(PLstr name, PLstr value)
{
    int rc = 0;
    void *wk;

    RX_IKJCT441_PARAMS_PTR params;

    /* do not handle special vars here */
    if (checkVariableBlacklist(name) != 0)
        return -1;

    /* NAME LENGTH < 1 OR > 252 */
    if (checkNameLength(name->len) != 0)
        return -2;

    params = malloc(sizeof(RX_IKJCT441_PARAMS));
    wk     = malloc(256);

    memset(wk,     0, sizeof(wk));
    memset(params, 0, sizeof(RX_IKJCT441_PARAMS));

    params->ecode    = 18;
    params->nameadr  = (char *)name->pstr;
    params->namelen  = name->len;
    params->valueadr = 0;
    params->valuelen = 0;
    params->wkadr    = wk;

    rc = call_rxikj441 (params);

    if (value->maxlen < params->valuelen) {
        Lfx(value,params->valuelen);
    }
    if (value->pstr != params->valueadr) {
        strncpy((char *)value->pstr,params->valueadr,params->valuelen);
    }

    value->len    = params->valuelen;
    value->maxlen = params->valuelen;
    value->type   = LSTRING_TY;

    free(wk);
    free(params);

    return rc;
}

int
SetClistVar(PLstr name, PLstr value)
{
    int rc = 0;
    void *wk;

    RX_IKJCT441_PARAMS_PTR params;

    /* convert numeric values to a string */
    if (value->type != LSTRING_TY) {
        L2str(value);
    }

    /* terminate all strings with a binary zero */
    LASCIIZ(*name);
    LASCIIZ(*value);

    /* do not handle special vars here */
    if (checkVariableBlacklist(name) != 0)
        return -1;

    /* NAME LENGTH < 1 OR > 252 */
    if (checkNameLength(name->len) != 0)
        return -2;

    /* VALUE LENGTH < 0 OR > 32767 */
    if (checkValueLength(value->len) != 0)
        return -3;

    params = malloc(sizeof(RX_IKJCT441_PARAMS));
    wk     = malloc(256);

    memset(wk,     0, sizeof(wk));
    memset(params, 0, sizeof(RX_IKJCT441_PARAMS)),

            params->ecode    = 2;
    params->nameadr  = (char *)name->pstr;
    params->namelen  = name->len;
    params->valueadr = (char *)value->pstr;
    params->valuelen = value->len;
    params->wkadr    = wk;

    rc = call_rxikj441(params);

    free(wk);
    free(params);

    return rc;
}

//----------------------------------------
// BLDL/FIND
//----------------------------------------
int findLoadModule(char *moduleName)
{
    int iRet = 0;
    char sTempÝ8¨;
    char *sToken;

    RX_BLDL_PARAMS bldlParams;
    RX_SVC_PARAMS svcParams;

    memset(&bldlParams, 0, sizeof(RX_BLDL_PARAMS));
    memset(&bldlParams.BLDLN, ' ', 8);

    strncpy(sTemp, moduleName, 8);

    sToken = strtok(sTemp, " ");
    strncpy(bldlParams.BLDLN, sToken, strlen(sToken));

    bldlParams.BLDLF = 1;
    bldlParams.BLDLL = 50;

    svcParams.SVC = 18;
    svcParams.R0  = (unsigned)&bldlParams;
    svcParams.R1  = 0;

    call_rxsvc(&svcParams);

    if (svcParams.R15 == 0) {
        iRet = 1;
    }

    return iRet;
}

/* internal functions */
int checkNameLength(long lName)
{
    int rc = 0;
    if (lName < 1)
        rc = -1;
    if (lName > 252)
        rc =  1;

    return rc;
}

int checkValueLength(long lValue)
{
    int rc = 0;

    if (lValue == 0)
        rc = -1;
    if (lValue > 32767)
        rc =  1;

    return rc;
}

int checkVariableBlacklist(PLstr name)
{
    int rc = 0;
    int i  = 0;

    Lupper(name);

    for (i = 0; i < BLACKLIST_SIZE; ++i) {
        if (strcmp((char *)name->pstr,RX_VAR_BLACKLISTÝi¨) == 0)
            return -1;
    }

    return rc;
}

