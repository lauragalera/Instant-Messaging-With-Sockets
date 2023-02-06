/**************************************************************************/
/*                                                                        */
/* P2 - MI amb sockets TCP/IP - Part II                                   */
/* Fitxer tUDP.c que "implementa" la capa de transport UDP, o més         */
/* ben dit, que encapsula les funcions de la interfície de sockets        */
/* UDP, en unes altres funcions més simples i entenedores: la "nova"      */
/* interfície de sockets UDP.                                             */
/* Autors: Laura Galera Alfaro i David Perez Sanchez                      */
/*                                                                        */
/**************************************************************************/

#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "MIp2-tUDP.h"

/* Crea un socket UDP a l’@IP “IPloc” i #port UDP “portUDPloc”            */
/* (si “IPloc” és “0.0.0.0” i/o “portUDPloc” és 0 es fa/farà una          */
/* assignació implícita de l’@IP i/o del #port UDP, respectivament).      */
/*                                                                        */
/* "IPloc" és un "string" de C (vector de chars imprimibles acabat en     */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0').               */
/*                                                                        */
/* Retorna -1 si hi ha error; l’identificador del socket creat si tot     */
/* va bé.                                                                 */
int UDP_CreaSock(const char *IPloc, int portUDPloc)
{
    struct sockaddr_in adr;
    int soc;
    int i;

    // Crear socket UDP
    if((soc=socket(AF_INET,SOCK_DGRAM,0))==-1) {
        return (-1);
    }

    // Configurar connexio
    adr.sin_family=AF_INET;
    adr.sin_port=htons(portUDPloc);
    adr.sin_addr.s_addr=inet_addr(IPloc);
    for(i=0;i<8;i++){adr.sin_zero[i]='\0';}

    // Configurar socket
    if((bind(soc,(struct sockaddr*)&adr,sizeof(adr)))==-1) {
        close(soc);
        return (-1);
    }

    return soc;
}

/* Envia a través del socket UDP d’identificador “Sck” la seqüència de    */
/* bytes escrita a “SeqBytes” (de longitud “LongSeqBytes” bytes) cap al   */
/* socket remot que té @IP “IPrem” i #port UDP “portUDPrem”.              */
/*                                                                        */
/* "IPrem" és un "string" de C (vector de chars imprimibles acabat en     */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0'); "SeqBytes"    */
/* és un vector de chars qualsevol (recordeu que en C, un char és un      */
/* enter de 8 bits) d'una longitud >= LongSeqBytes bytes.                 */
/*                                                                        */
/* Retorna -1 si hi ha error; el nombre de bytes enviats si tot va bé.    */
int UDP_EnviaA(int Sck, const char *IPrem, int portUDPrem, const char *SeqBytes, int LongSeqBytes)
{
    int i, bescrit;
    struct sockaddr_in adr;

    adr.sin_family=AF_INET;
    adr.sin_port=htons(portUDPrem);
    adr.sin_addr.s_addr= inet_addr(IPrem);
    for(i=0;i<8;i++){adr.sin_zero[i]='\0';}
    if((bescrit=sendto(Sck,SeqBytes,LongSeqBytes,0,(struct sockaddr*)&adr,sizeof(adr)))==-1)
    {
        return(-1);
    }

    return bescrit;
}

/* Rep a través del socket UDP d’identificador “Sck” una seqüència de     */
/* bytes que prové d'un socket remot i l’escriu a “SeqBytes*” (que té     */
/* una longitud de “LongSeqBytes” bytes).                                 */
/*                                                                        */
/* Omple "IPrem" i "portUDPrem" amb respectivament, l'@IP i el #port      */
/* UDP del socket remot.                                                  */
/*                                                                        */
/* "IPrem" és un "string" de C (vector de chars imprimibles acabat en     */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0'); "SeqBytes"    */
/* és un vector de chars qualsevol (recordeu que en C, un char és un      */
/* enter de 8 bits) d'una longitud <= LongSeqBytes bytes.                 */
/*                                                                        */
/* Retorna -1 si hi ha error; el nombre de bytes rebuts si tot va bé.     */
int UDP_RepDe(int Sck, char *IPrem, int *portUDPrem, char *SeqBytes, int LongSeqBytes)
{
    int ladr, bllegit;
    struct sockaddr_in adr;

    ladr = sizeof(adr);
    if((bllegit=recvfrom(Sck,SeqBytes,LongSeqBytes,0,(struct sockaddr*)&adr,&ladr))==-1)  {
        return(-1);
    }

    // Omplir parametres d'entrada
    strcpy(IPrem,inet_ntoa(adr.sin_addr));
    *portUDPrem = ntohs(adr.sin_port);

    return bllegit;
}

/* S’allibera (s’esborra) el socket UDP d’identificador “Sck”.            */
/*                                                                        */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé.    */
int UDP_TancaSock(int Sck)
{
    return close(Sck);
}

/* Donat el socket UDP d’identificador “Sck”, troba l’adreça d’aquest     */
/* socket, omplint “IPloc” i “portUDPloc” amb respectivament, la seva     */
/* @IP i #port UDP.                                                       */
/*                                                                        */
/* "IPloc" és un "string" de C (vector de chars imprimibles acabat en     */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0').               */
/*                                                                        */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé.    */
int UDP_TrobaAdrSockLoc(int Sck, char *IPloc, int *portUDPloc)
{
    struct sockaddr_in adr;
    socklen_t long_adr = sizeof(adr);

    // Obtenir info del socket local
    if(getsockname(Sck, (struct sockaddr *)&adr, &long_adr) == -1) {
        return (-1);
    }

    // Omplir parametres d'entrada
    strcpy(IPloc,inet_ntoa(adr.sin_addr));
    *portUDPloc = ntohs(adr.sin_port);

    return 1;
}

/* El socket UDP d’identificador “Sck” es connecta al socket UDP d’@IP    */
/* “IPrem” i #port UDP “portUDPrem” (si tot va bé es diu que el socket    */
/* “Sck” passa a l’estat “connectat” o establert – established –).        */
/*                                                                        */
/* Recordeu que a UDP no hi ha connexions com a TCP, i que això només     */
/* vol dir que es guarda localment l’adreça “remota” i així no cal        */
/* especificar-la cada cop per enviar i rebre. Llavors quan un socket     */
/* UDP està “connectat” es pot fer servir UDP_Envia() i UDP_Rep() (a més  */
/* de les anteriors UDP_EnviaA() i UDP_RepDe()) i UDP_TrobaAdrSockRem()). */
/*                                                                        */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé.    */
int UDP_DemanaConnexio(int Sck, const char *IPrem, int portUDPrem)
{
    int i;
    struct sockaddr_in adr;

    adr.sin_family=AF_INET;
    adr.sin_port=htons(portUDPrem);
    adr.sin_addr.s_addr= inet_addr(IPrem);
    for(i=0;i<8;i++){adr.sin_zero[i]='\0';}

    if(connect(Sck,(struct sockaddr*)&adr,sizeof(adr)) == -1)
        return (-1);
    else
        return (1);
}

/* Envia a través del socket UDP “connectat” d’identificador “Sck” la     */
/* seqüència de bytes escrita a “SeqBytes” (de longitud “LongSeqBytes”    */
/* bytes) cap al socket UDP remot amb qui està connectat.                 */
/*                                                                        */
/* "SeqBytes" és un vector de chars qualsevol (recordeu que en C, un      */
/* char és un enter de 8 bits) d'una longitud >= LongSeqBytes bytes.      */
/*                                                                        */
/* Retorna -1 si hi ha error; el nombre de bytes enviats si tot va bé.    */
int UDP_Envia(int Sck, const char *SeqBytes, int LongSeqBytes)
{
    int bytes_escrits;

    // Escriure al socket les dades
    if ((bytes_escrits = write(Sck, SeqBytes, LongSeqBytes)) == -1) {
        return (-1);
    }

    return bytes_escrits;
}

/* Rep a través del socket UDP “connectat” d’identificador “Sck” una      */
/* seqüència de bytes que prové del socket remot amb qui està connectat,  */
/* i l’escriu a “SeqBytes” (que té una longitud de “LongSeqBytes” bytes). */
/*                                                                        */
/* "SeqBytes" és un vector de chars qualsevol (recordeu que en C, un      */
/* char és un enter de 8 bits) d'una longitud <= LongSeqBytes bytes.      */
/*                                                                        */
/* Retorna -1 si hi ha error; el nombre de bytes rebuts si tot va bé.     */
int UDP_Rep(int Sck, char *SeqBytes, int LongSeqBytes)
{
    int bytes_llegits;

    // Llegir del socket les dades
    if((bytes_llegits=read(Sck,SeqBytes,LongSeqBytes))==-1){
        return (-1);
    }

    return bytes_llegits;
}

/* Donat el socket UDP “connectat” d’identificador “Sck”, troba l’adreça  */
/* del socket remot amb qui està connectat, omplint “IPrem” i             */
/* “portUDPrem” amb respectivament, la seva @IP i #port UDP.              */
/*                                                                        */
/* "IPrem" és un "string" de C (vector de chars imprimibles acabat en     */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0').               */
/*                                                                        */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé.    */
int UDP_TrobaAdrSockRem(int Sck, char *IPrem, int *portUDPrem)
{
    struct sockaddr_in adr;
    socklen_t long_adr = sizeof(adr);

    // Obtenir info del socket local
    if(getpeername(Sck, (struct sockaddr *)&adr, &long_adr) == -1) {
        return (-1);
    }

    // Omplir parametres d'entrada
    strcpy(IPrem,inet_ntoa(adr.sin_addr));
    *portUDPrem = ntohs(adr.sin_port);

    return 1;
}

/* Obté un missatge de text que descriu l'error produït en la darrera     */
/* crida de sockets UDP.                                                  */
/*                                                                        */
/* Retorna aquest missatge de text en un "string" de C (vector de chars   */
/* imprimibles acabat en '\0')                                            */
char* UDP_ObteMissError(void)
{
 return strerror(errno);
}