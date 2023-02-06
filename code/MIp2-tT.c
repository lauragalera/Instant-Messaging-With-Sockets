/**************************************************************************/
/*                                                                        */
/* P2 - MI amb sockets TCP/IP - Part II                                   */
/* Fitxer tT.c que "implementa" la part "compartida" de les capes         */
/* de transport TCP i UDP, o més ben dit, que encapsula les funcions de   */
/* la interfície de sockets TCP o UDP "compartides" (en el sentit, que    */
/* són funcions que poden tractar alhora sockets TCP i UDP), en unes      */
/* altres funcions més simples i entenedores: la "nova" interfície de     */
/* sockets.                                                               */
/* Autors: Laura Galera Alfaro i David Perez Sanchez                      */
/*                                                                        */
/**************************************************************************/

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include "MIp2-tT.h"

/* Examina simultàniament i sense límit de temps (una espera indefinida)  */
/* els sockets (poden ser TCP, UDP i  teclat -stdin-) amb identificadors  */
/* en la llista “LlistaSck” (de longitud “LongLlistaSck” sockets) per     */
/* saber si hi ha arribat alguna cosa per ser llegida.                    */
/*                                                                        */
/* "LlistaSck" és un vector d'enters d'una longitud >= LongLlistaSck.     */
/*                                                                        */
/* Retorna -1 si hi ha error; si arriba alguna cosa per algun dels        */
/* sockets, retorna l’identificador d’aquest socket.                      */
int T_HaArribatAlgunaCosa(const int *LlistaSck, int LongLlistaSck)
{
    return T_HaArribatAlgunaCosaEnTemps(LlistaSck,LongLlistaSck,-1);
}

/* Examina simultàniament durant "Temps" (en [ms]) els sockets (poden ser */
/* TCP, UDP i teclat -stdin-) amb identificadors en la llista “LlistaSck” */
/* (de longitud “LongLlistaSck” sockets) per saber si hi ha arribat       */
/* alguna cosa per ser llegida. Si Temps és -1, s'espera indefinidament   */
/* fins que arribi alguna cosa.                                           */
/*                                                                        */
/* "LlistaSck" és un vector d'enters d'una longitud >= LongLlistaSck.     */
/*                                                                        */
/* Retorna -1 si hi ha error; retorna -2 si passa "Temps" sense que       */
/* arribi res; si arriba alguna cosa per algun dels sockets, retorna      */
/* l’identificador d’aquest socket.                                       */
/*                                                                        */
/* (aquesta funció podria substituir a l'anterior T_HaArribatAlgunaCosa() */
/* ja que quan “Temps” és -1 és equivalent a ella)                        */
int T_HaArribatAlgunaCosaEnTemps(const int *LlistaSck, int LongLlistaSck, int Temps)
{
    fd_set conjunt;
    int descmax = -1;
    int i;
    struct timeval tv = {0,0};

    // Preparar variables pel select()
    FD_ZERO(&conjunt);
    for (i = 0; i < LongLlistaSck; ++i) {
        FD_SET(LlistaSck[i], &conjunt);
        if(LlistaSck[i] > descmax)
            descmax = LlistaSck[i];
    }

    if(Temps == -1){
        // Esperar que arribin dades per algun dels FD
        if(select(descmax+1, &conjunt, NULL, NULL, NULL) == -1) {
            return (-1);
        }
    }
    else {
        // Esperar que arribin dades per algun dels FD amb el timeout indicat
        tv.tv_usec = Temps * 1000;
        int res = select(descmax+1, &conjunt, NULL, NULL, &tv);
        if(res == -1) {
            return (-1);
        }
        else if(res == 0) return (-2);
    }

    // Trobar quin FD ha rebut dades
    int fd = -1;
    i = 0;
    while (fd == -1 && i < LongLlistaSck) {
        if(FD_ISSET(LlistaSck[i],&conjunt))
            fd = LlistaSck[i];
        i++;
    }

    return fd;
}

/* Obté un missatge de text que descriu l'error produït en la darrera     */
/* crida de sockets (de la part "compartida" de transport).               */
/*                                                                        */
/* Retorna aquest missatge de text en un "string" de C (vector de chars   */
/* imprimibles acabat en '\0')                                            */
char* T_ObteMissError(void)
{
 return strerror(errno);
}