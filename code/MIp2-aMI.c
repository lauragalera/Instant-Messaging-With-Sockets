/**************************************************************************/
/*                                                                        */
/* P1 - MI amb sockets TCP/IP - Part I                                    */
/* Fitxer mi.c que implementa la capa d'aplicació de MI, sobre la capa de */
/* transport TCP (fent crides a la "nova" interfície de la capa TCP o     */
/* "nova" interfície de sockets)                                          */
/* Autors: Laura Galera Alfaro, David Pérez Sánchez                       */
/*                                                                        */
/**************************************************************************/

#include "MIp2-aMI.h"
#include "MIp2-tTCP.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int enviarPaq(const char tipus, const int sck, const char * info, const int long_info);
int rebreNick(const int sck, char * info, int * long_info);
int rebrePeticio(const int sck, char * info, int * long_info);

/* Inicia l’escolta de peticions remotes de conversa a través d’un nou    */
/* socket TCP, que tindrà una @IP local qualsevol i el #port TCP          */
/* “portTCPloc” (és a dir, crea un socket “servidor” o en estat d’escolta */
/* – listen –).                                                           */
/*                                                                        */
/* Retorna -1 si hi ha error; l’identificador del socket d’escolta de MI  */
/* creat si tot va bé.                                                    */
int MI_IniciaEscPetiRemConv(int portTCPloc)
{
    int sesc;

    // Crear el socket sesc d'escolta i preparar-lo per escoltar peticions
    if((sesc = TCP_CreaSockServidor("0.0.0.0",portTCPloc)) == -1) {
        return(-1);
    }

    return sesc;
}

/* Crea una conversa iniciada per una petició local que arriba a través   */
/* del teclat: crea un socket TCP “client” (en un #port TCP i @IP local   */
/* qualsevol), a través del qual fa una petició de conversa a un procés   */
/* remot, el qual les escolta a través del socket TCP ("servidor") d'@IP  */
/* “IPrem” i #port TCP “portTCPrem” (és a dir, crea un socket “connectat” */
/* o en estat establert – established –). Aquest socket serà el que es    */
/* farà servir durant la conversa.                                        */
/*                                                                        */
/* Omple “IPloc” i “portTCPloc” amb, respectivament, l’@IP i el #port     */
/* TCP del socket del procés local.                                       */
/*                                                                        */
/* El nickname local “NicLoc” i el nickname remot són intercanviats amb   */
/* el procés remot, i s’omple “NickRem” amb el nickname remot. El procés  */
/* local és qui inicia aquest intercanvi (és a dir, primer s’envia el     */
/* nickname local i després es rep el nickname remot).                    */
/*                                                                        */
/* "IPrem" i "IPloc" són "strings" de C (vectors de chars imprimibles     */
/* acabats en '\0') d'una longitud màxima de 16 chars (incloent '\0').    */
/* "NicLoc" i "NicRem" són "strings" de C (vectors de chars imprimibles   */
/* acabats en '\0') d'una longitud màxima de 100 chars (incloent '\0').   */
/*                                                                        */
/* Retorna -1 si hi ha error; l’identificador del socket de conversa de   */
/* MI creat si tot va bé.                                                 */
int MI_DemanaConv(const char *IPrem, int portTCPrem, char *IPloc, int *portTCPloc, const char *NicLoc, char *NicRem)
{
    int scon;

    // Crear scon
    if((scon = TCP_CreaSockClient("0.0.0.0",0)) == -1) {
        return(-1);
    }

    // Connectar amb el servidor
    if(TCP_DemanaConnexio(scon,IPrem,portTCPrem) == -1) {
        return(-1);
    }

    // Trobar IP i port locals
    if(TCP_TrobaAdrSockLoc(scon,IPloc,portTCPloc) == -1) {
        return(-1);
    }

    // Intercanvi de nicknames
    if (enviarPaq('N',scon,NicLoc,strlen(NicLoc)) == -1) {
        return(-1);
    }
    int long_NickRem;
    if ((rebreNick(scon,NicRem,&long_NickRem)) == -1) {
        return(-1);
    }
    NicRem[long_NickRem] = '\0';

    return scon;
}

/* Crea una conversa iniciada per una petició remota que arriba a través  */
/* del socket d’escolta de MI d’identificador “SckEscMI” (un socket       */
/* “servidor”): accepta la petició i crea un socket (un socket            */
/* “connectat” o en estat establert – established –), que serà el que es  */
/* farà servir durant la conversa.                                        */
/*                                                                        */
/* Omple “IPrem”, “portTCPrem”, “IPloc” i “portTCPloc” amb,               */
/* respectivament, l’@IP i el #port TCP del socket del procés remot i del */
/* socket del procés local.                                               */
/*                                                                        */
/* El nickname local “NicLoc” i el nickname remot són intercanviats amb   */
/* el procés remot, i s’omple “NickRem” amb el nickname remot. El procés  */
/* remot és qui inicia aquest intercanvi (és a dir, primer es rep el      */
/* nickname remot i després s’envia el nickname local).                   */
/*                                                                        */
/* "IPrem" i "IPloc" són "strings" de C (vectors de chars imprimibles     */
/* acabats en '\0') d'una longitud màxima de 16 chars (incloent '\0').    */
/*                                                                        */
/* "NicLoc" i "NicRem" són "strings" de C (vectors de chars imprimibles   */
/* acabats en '\0') d'una longitud màxima de 100 chars (incloent '\0').   */
/*                                                                        */
/* Retorna -1 si hi ha error; l’identificador del socket de conversa      */
/* de MI creat si tot va bé.                                              */
int MI_AcceptaConv(int SckEscMI, char *IPrem, int *portTCPrem, char *IPloc, int *portTCPloc, const char *NicLoc, char *NicRem)
{
    int scon;

    // Acceptar connexio
    if((scon = TCP_AcceptaConnexio(SckEscMI,IPrem,portTCPrem)) == -1) {
        return(-1);
    }

    // Trobar IP i port locals
    if(TCP_TrobaAdrSockLoc(scon,IPloc,portTCPloc) == -1) {
        return(-1);
    }

    // Intercanvi de nicknames
    int long_NickRem;
    if ((rebreNick(scon,NicRem,&long_NickRem)) == -1) {
        return(-1);
    }
    NicRem[long_NickRem] = '\0';
    if (enviarPaq('N',scon,NicLoc,strlen(NicLoc)) == -1) {
        return(-1);
    }

    return scon;
}

/* A través del socket de conversa de MI d’identificador “SckConvMI” (un  */
/* socket "connectat") envia una línia escrita per l’usuari local, els    */
/* caràcters i el nombre de caràcters de la qual es troben a “Linia” i    */
/* “LongLinia”, respectivament                                            */
/*                                                                        */
/* "Linia" no és un "string" de C sinó un vector de chars imprimibles,    */
/* sense el caràter fi d'string ('\0'); tampoc conté el caràcter fi de    */
/* línia ('\n') i cal que 0<= LongLinia <= 99.                            */
/*                                                                        */
/* Retorna -1 si hi ha error; el nombre de caràcters de la línia enviada  */
/* si tot va bé.                                                          */
int MI_EnviaLinia(int SckConvMI, const char *Linia, int LongLinia)
{
    return enviarPaq('L',SckConvMI,Linia,LongLinia);
}

/* A través del socket de conversa de MI d’identificador “SckConvMI” (un  */
/* socket "connectat") atén una petició de servei de l’usuari remot, i    */
/* omple “SeqBytes” i “LongSeqBytes” amb una “informació” que depèn del   */
/* tipus de petició.                                                      */
/*                                                                        */
/* “LongSeqBytes”, abans de cridar la funció, ha de contenir la longitud  */
/* de la seqüència de bytes “SeqBytes”, que ha de ser >= 99 (després de   */
/* cridar-la, contindrà la longitud en bytes d’aquella “informació”).     */
/*                                                                        */
/* Atendre una petició de servei pot ser: rebre una línia escrita per     */
/* l’usuari remot i llavors omplir “SeqBytes” i “LongSeqBytes” amb els    */
/* caràcters i el nombre de caràcters de la línia rebuda, respectivament; */
/* o bé servir un altre tipus de petició de servei (a definir); o bé      */
/* detectar l’acabament de la conversa per part de l’usuari remot.        */
/*                                                                        */
/* Retorna -1 si hi ha error; 0 si l’usuari remot acaba la conversa; 1 si */
/* rep una línia; 2, 3, etc., si serveix un altre tipus de petició de     */
/* servei (a definir).                                                    */
int MI_ServeixPeticio(int SckConvMI, char *SeqBytes, int *LongSeqBytes)
{
    int tipus = rebrePeticio(SckConvMI,SeqBytes,LongSeqBytes);
    if(tipus != -1 && tipus != 0) {
        SeqBytes[*LongSeqBytes] = '\0';
        (*LongSeqBytes)++;
    }
    return tipus;
}

/* Acaba la conversa associada al socket de conversa de MI                */
/* d’identificador “SckConvMI” (un socket “connectat”).                   */
/*                                                                        */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé.    */
int MI_AcabaConv(int SckConvMI)
{
    return TCP_TancaSock(SckConvMI);
}

/* Acaba l’escolta de peticions remotes de conversa que arriben a través  */
/* del socket d’escolta de MI d’identificador “SckEscMI” (un socket       */
/* “servidor”).                                                           */
/*                                                                        */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé.    */
int MI_AcabaEscPetiRemConv(int SckEscMI)
{
    return TCP_TancaSock(SckEscMI);
}

/* Obté un missatge de text que descriu l'error produït en la darrera     */
/* crida de sockets.                                                      */
/*                                                                        */
/* Retorna aquest missatge de text en un "string" de C (vector de chars   */
/* imprimibles acabat en '\0')                                            */
/*                                                                        */
/* (aquesta funció és equivalent a la funció T_MostraError() de la "nova" */
/* interfície de la capa TCP però ara a la capa d’aplicació)              */
char* MI_ObteMissError(void)
{
    return TCP_ObteMissError();
}

/* Donat el socket d’identificador “Sck”, troba l’adreça d’aquest
 * socket, omplint “IPloc” i “portTCPloc” amb respectivament, la seva
 * IP i port.
 *
 * Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé.
 */
int MI_TrobarAdreca(int Sck, char * IPloc, int * portTCPloc)
{
    return TCP_TrobaAdrSockLoc(Sck,IPloc,portTCPloc);
}

/* Es crea i s'envia el paquet amb el tipus ("tipus"), la longitud ("long_info") i el
 * camp informació ("info") a través del socket connectat amb identificador "sck".
 *
 * Es retorna -1 si l'enviament no s'ha fet correctament; la longitud total
 * del paquet altrament.
 */
int enviarPaq(const char tipus, const int sck, const char * info, const int long_info) {
    int long_paquet = 3 + long_info;
    char paquet[long_paquet];
    char aux[long_info+1];

    // Generar paquet
    strcpy(aux,info);
    aux[long_info] = '\0';
    sprintf(paquet,"%c%02d%s",tipus,long_info,aux);

    // Enviar paquet
    if (TCP_Envia(sck, paquet, long_paquet) == -1) {
        TCP_ObteMissError();
        return(-1);
    }

    return long_paquet;
}

/* Es rep a través del socket connectat i identificat per
 * "sck" una línia de missatge de l'usuari remot i es guarda el valor
 *  a "info" i la llargada a "long_info".
 *
 * Retorna -1 si hi ha error, 0 si s'ha tancat la connexio i 1 si s'ha rebut una Linia.
 */
int rebrePeticio(const int sck, char * info, int * long_info) {
    char paquet[102];

    // Rebre paquet
    int bytes_llegits = TCP_Rep(sck,paquet,sizeof(paquet));
    if(bytes_llegits == -1) {
        return(-1);
    }
    else if(bytes_llegits == 0) {
        return(0);
    }

    // Obtenir tipus de paquet
    char tipus = paquet[0];

    // Obtenir la longitud
    char aux[2];
    memcpy(aux,&paquet[1],2);
    *long_info = strtol(aux,NULL,10);

    // Obtenir informacio
    memcpy(info,&paquet[3],*long_info);

    // Retornar resultat
    switch (tipus) {
        case 'L':
            return 1;
            break;
        default:
            return -1;
    }
}

/* Es rep a través del socket connectat i identificat per
 * "sck" el nickname de l'usuari remot i es guarda el valor
 *  a "info" i la llargada a "long_info".
 *
 * Retorna -1 si hi ha error o si s'ha tancat la connexio i 1 si
 * ha anat be.
 */
int rebreNick(const int sck, char * info, int * long_info) {
    char paquet[102];

    // Rebre paquet
    int bytes_llegits = TCP_Rep(sck,paquet,sizeof(paquet));
    if(bytes_llegits == -1) {
        return(-1);
    }
    else if(bytes_llegits == 0) {
        return(-1);
    }

    // Obtenir la longitud
    char aux[2];
    memcpy(aux,&paquet[1],2);
    *long_info = strtol(aux,NULL,10);

    // Obtenir informacio
    memcpy(info,&paquet[3],*long_info);

    return 1;
}

