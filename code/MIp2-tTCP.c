/**************************************************************************/
/*                                                                        */
/* P1 - MI amb sockets TCP/IP - Part II                                   */
/* Fitxer t.c que "implementa" la capa de transport, o més ben dit, que   */
/* encapsula les funcions de la interfície de sockets, en unes altres     */
/* funcions més simples i entenedores: la "nova" interfície de sockets.   */
/* Autors: David Pérez Sánchez, Laura Galera Alfaro                       */
/*                                                                        */
/**************************************************************************/

#include "MIp2-tTCP.h"
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/ioctl.h>

/* Crea un socket TCP “client” a l’@IP “IPloc” i #port TCP “portTCPloc”   */
/* (si “IPloc” és “0.0.0.0” i/o “portTCPloc” és 0 es fa/farà una          */
/* assignació implícita de l’@IP i/o del #port TCP, respectivament).      */
/* "IPloc" és un "string" de C (vector de chars imprimibles acabat en     */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0').               */
/* Retorna -1 si hi ha error; l’identificador del socket creat si tot     */
/* va bé.                                                                 */
int TCP_CreaSockClient(const char *IPloc, int portTCPloc)
{
    struct sockaddr_in adr;
    int soc;
    int i;

    // Crear socket TCP
    if((soc=socket(AF_INET,SOCK_STREAM,0))==-1) {
        return (-1);
    }

    // Configurar connexio
    adr.sin_family=AF_INET;
    adr.sin_port=htons(portTCPloc);
    adr.sin_addr.s_addr=inet_addr(IPloc);
    for(i=0;i<8;i++){adr.sin_zero[i]='\0';}

    // Configurar socket
    if((bind(soc,(struct sockaddr*)&adr,sizeof(adr)))==-1) {
        close(soc);
        return (-1);
    }

    return soc;
}

/* Crea un socket TCP “servidor” (o en estat d’escolta – listen –) a      */
/* l’@IP “IPloc” i #port TCP “portTCPloc” (si “IPloc” és “0.0.0.0” i/o    */
/* “portTCPloc” és 0 es fa una assignació implícita de l’@IP i/o del      */
/* #port TCP, respectivament).                                            */
/* "IPloc" és un "string" de C (vector de chars imprimibles acabat en     */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0').               */
/* Retorna -1 si hi ha error; l’identificador del socket creat si tot     */
/* va bé.                                                                 */
int TCP_CreaSockServidor(const char *IPloc, int portTCPloc)
{
    // Crear socket
    int soc;
    if((soc = TCP_CreaSockClient(IPloc,portTCPloc)) == -1)
        return (-1);

    // Crear cua per emmagatzemar peticions de connexio pendents
    if((listen(soc,3))==-1) {
        close(soc);
        return(-1);
    }

    return soc;
}

/* El socket TCP “client” d’identificador “Sck” es connecta al socket     */
/* TCP “servidor” d’@IP “IPrem” i #port TCP “portTCPrem” (si tot va bé    */
/* es diu que el socket “Sck” passa a l’estat “connectat” o establert     */
/* – established –). Recordeu que això vol dir que s’estableix una        */
/* connexió TCP (les dues entitats TCP s’intercanvien missatges           */
/* d’establiment de la connexió).                                         */
/* "IPrem" és un "string" de C (vector de chars imprimibles acabat en     */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0').               */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé.    */
int TCP_DemanaConnexio(int Sck, const char *IPrem, int portTCPrem)
{
    int i;
    struct sockaddr_in adr;

    // Configurar connexio
    adr.sin_family=AF_INET;
    adr.sin_port=htons(portTCPrem);
    adr.sin_addr.s_addr=inet_addr(IPrem);
    for(i=0;i<8;i++){adr.sin_zero[i]='\0';}

    // Demanar connexio al servidor
    if((connect(Sck,(struct sockaddr*)&adr,sizeof(adr)))==-1){
        return (-1);
    }

    return 1;
}

/* El socket TCP “servidor” d’identificador “Sck” accepta fer una         */
/* connexió amb un socket TCP “client” remot, i crea un “nou” socket,     */
/* que és el que es farà servir per enviar i rebre dades a través de la   */
/* connexió (es diu que aquest nou socket es troba en l’estat “connectat” */
/* o establert – established –; el nou socket té la mateixa adreça que    */
/* “Sck”).                                                                */
/* Omple “IPrem*” i “portTCPrem*” amb respectivament, l’@IP i el #port    */
/* TCP del socket remot amb qui s’ha establert la connexió.               */
/* "IPrem*" és un "string" de C (vector de chars imprimibles acabat en    */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0').               */
/* Retorna -1 si hi ha error; l’identificador del socket connectat creat  */
/* si tot va bé.                                                          */
int TCP_AcceptaConnexio(int Sck, char *IPrem, int *portTCPrem)
{
    int scon;
    struct sockaddr_in adr;
    unsigned long_adr = sizeof(adr);

    // Acceptar la sol.licitud de connexio del client i crear el socket
    // d'intercanvi de dades
    if((scon=accept(Sck,(struct sockaddr*)&adr, &long_adr))==-1) {
        return (-1);
    }

    // Omplir parametres d'entrada
    strcpy(IPrem,inet_ntoa(adr.sin_addr));
    *portTCPrem = ntohs(adr.sin_port);

    return scon;
}

/* Envia a través del socket TCP “connectat” d’identificador “Sck” la     */
/* seqüència de bytes escrita a “SeqBytes” (de longitud “LongSeqBytes”    */
/* bytes) cap al socket TCP remot amb qui està connectat.                 */
/* "SeqBytes" és un vector de chars qualsevol (recordeu que en C, un      */
/* char és un enter de 8 bits) d'una longitud >= LongSeqBytes bytes.      */
/* Retorna -1 si hi ha error; el nombre de bytes enviats si tot va bé.    */
int TCP_Envia(int Sck, const char *SeqBytes, int LongSeqBytes)
{
    int bytes_escrits;

    // Escriure al socket les dades
    if ((bytes_escrits = write(Sck, SeqBytes, LongSeqBytes)) == -1) {
        return (-1);
    }

    return bytes_escrits;
}

/* Rep a través del socket TCP “connectat” d’identificador “Sck” una      */
/* seqüència de bytes que prové del socket remot amb qui està connectat,  */
/* i l’escriu a “SeqBytes*” (que té una longitud de “LongSeqBytes” bytes),*/
/* o bé detecta que la connexió amb el socket remot ha estat tancada.     */
/* "SeqBytes*" és un vector de chars qualsevol (recordeu que en C, un     */
/* char és un enter de 8 bits) d'una longitud <= LongSeqBytes bytes.      */
/* Retorna -1 si hi ha error; 0 si la connexió està tancada; el nombre de */
/* bytes rebuts si tot va bé.                                             */
int TCP_Rep(int Sck, char *SeqBytes, int LongSeqBytes)
{
    int bytes_llegits;

    // Llegir del socket les dades
    if((bytes_llegits=read(Sck,SeqBytes,LongSeqBytes))==-1){
        return (-1);
    }

    return bytes_llegits;
}

/* S’allibera (s’esborra) el socket TCP d’identificador “Sck”; si “Sck”   */
/* està connectat es tanca la connexió TCP que té establerta.             */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé.    */
int TCP_TancaSock(int Sck)
{
    return close(Sck);
}

/* Donat el socket TCP d’identificador “Sck”, troba l’adreça d’aquest     */
/* socket, omplint “IPloc*” i “portTCPloc*” amb respectivament, la seva   */
/* @IP i #port TCP.                                                       */
/* "IPloc*" és un "string" de C (vector de chars imprimibles acabat en    */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0').               */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé.    */
int TCP_TrobaAdrSockLoc(int Sck, char *IPloc, int *portTCPloc)
{
    struct sockaddr_in adr;
    socklen_t long_adr = sizeof(adr);

    // Obtenir info del socket local
    if(getsockname(Sck, (struct sockaddr *)&adr, &long_adr) == -1) {
        return (-1);
    }

    // Omplir parametres d'entrada
    strcpy(IPloc,inet_ntoa(adr.sin_addr));
    *portTCPloc = ntohs(adr.sin_port);

    return 1;
}

/* Donat el socket TCP “connectat” d’identificador “Sck”, troba l’adreça  */
/* del socket remot amb qui està connectat, omplint “IPrem*” i            */
/* “portTCPrem*” amb respectivament, la seva @IP i #port TCP.             */
/* "IPrem*" és un "string" de C (vector de chars imprimibles acabat en    */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0').               */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé.    */
int TCP_TrobaAdrSockRem(int Sck, char *IPrem, int *portTCPrem)
{
    struct sockaddr_in adr;
    socklen_t long_adr = sizeof(adr);

    // Obtenir info del socket local
    if(getpeername(Sck, (struct sockaddr *)&adr, &long_adr) == -1) {
        return (-1);
    }

    // Omplir parametres d'entrada
    strcpy(IPrem,inet_ntoa(adr.sin_addr));
    *portTCPrem = ntohs(adr.sin_port);

    return 1;
}

/* Obté un missatge de text que descriu l'error produït en la darrera     */
/* crida de sockets.                                                      */
/* Retorna aquest missatge de text en un "string" de C (vector de chars   */
/* imprimibles acabat en '\0')                                            */
char* TCP_ObteMissError(void)
{
    return strerror(errno);
}