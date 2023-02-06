/**************************************************************************/
/*                                                                        */
/* P2 - MI amb sockets TCP/IP - Part II                                   */
/* Fitxer aDNSc.c que "implementa" la capa d'aplicació DNS (part          */
/* client), o més ben dit, que encapsula les funcions de la interfície    */
/* de sockets de la part DNS, en unes altres funcions més simples i       */
/* entenedores: la"nova" interfície de la capa DNS, en la part client.    */
/* Autors: Laura Galera Alfaro i David Pérez Sánchez                      */
/*                                                                        */
/**************************************************************************/

#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>

//* Definició de constants, p.e.,                                          */

/* #define XYZ       1500                                                 */

/* Donat el nom DNS "NomDNS" obté la corresponent @IP i l'escriu a "IP"   */
/*                                                                        */
/* "NomDNS" és un "string" de C (vector de chars imprimibles acabat en    */
/* '\0') d'una longitud qualsevol, i "IP" és un "string" de C (vector de  */
/* chars imprimibles acabat en '\0') d'una longitud màxima de 16 chars    */
/* (incloent '\0').                                                       */
/*                                                                        */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé     */
int DNSc_ResolDNSaIP(const char *NomDNS, char *IP)
{
	struct hostent * dadesHost;
	struct in_addr adrHost;

	dadesHost = gethostbyname(NomDNS);
	if(dadesHost == NULL)
        return (-1);

	adrHost.s_addr = *((unsigned long *)dadesHost->h_addr_list[0]);
	strcpy(IP,(char*)inet_ntoa(adrHost));
    return 1;
}

/* Obté un missatge de text que descriu l'error produït en la darrera     */
/* crida de sockets (de la part client de DNS).                           */
/*                                                                        */
/* Retorna aquest missatge de text en un "string" de C (vector de chars   */
/* imprimibles acabat en '\0')                                            */
/*                                                                        */
const char* DNSc_ObteMissError(void)
{
 return hstrerror(errno);
}

