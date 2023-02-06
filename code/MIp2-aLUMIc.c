/**************************************************************************/
/*                                                                        */
/* P2 - MI amb sockets TCP/IP - Part II                                   */
/* Fitxer aLUMIc.c que implementa la capa d'aplicació de MI, sobre        */
/* la capa de transport UDP (fent crides a la "nova" interfície de la     */
/* capa UDP o "nova" interfície de sockets UDP), en la part client.       */
/* Autors: Laura Galera Alfaro i David Pérez Sánchez                      */
/*                                                                        */
/**************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "MIp2-tUDP.h"
#include "MIp2-tT.h"
#include "MIp2-aDNSc.h"

const int MIDA_PAQUET = 100;
const int TIMEOUT = 1000; // ms
const int MAX_ATTEMPTS = 5;
const int PORT_NODE = 3344;

char IP_NODE[16];

int Log_CreaFitx(const char *NomFitxLog);
int Log_Escriu(int FitxLog, const char *MissLog);
int Log_TancaFitx(int FitxLog);

int enviarRegDesreg(int Sck, const char * paquet, int fitxerLog);
int enviarPaquet(int Sck, const char * paquet, char * paquetResposta, int fitxerLog);
int escriureLog(int fitxerLog, const char enviatRebut, const char * paquet, int bytes);
int obtenirDomini(const char * adrecaMI, char * domini);
int obtenirIPport(const char * paquet, char * IP, int * port);


/* Inicia el socket LUMI amb identificador Sck a través d'un nou socket UDP, que tindrà una IP
 * i un port qualsevols.
 *
 * A la vegada, crea un fitxer de log amb nom nomFitxer (cadena de caràcters
 * acabada en \0 i longitud <= 36) i guarda el nou identificador del fitxer
 * a fitxerLog.
 *
 * Retorna -1 si hi ha un error; 0 si tot va correctament.
 */
int LUMIc_IniciaClient(int * Sck, int * fitxerLog, const char * nomFitxer) {
    // Crear socket LUMI
    if ((*Sck = UDP_CreaSock("0.0.0.0", 0)) == -1) {
        return (-1);
    }

    // Crear fitxer de Log
    if((*fitxerLog = Log_CreaFitx(nomFitxer)) == -1) {
        return (-1);
    }
    return (0);
}

/* Tanca el socket LUMI amb identficador Sck i també el fitxer amb identificador
 * fitxerLog.
 *
 * Retorna -1 si hi ha un error; 0 si tot va correctament.
 */
int LUMIc_FinalitzaClient(int Sck, int fitxerLog) {
    if(UDP_TancaSock(Sck) == -1) {
        return (-1);
    }
    if(Log_TancaFitx(fitxerLog) == -1) {
        return (-1);
    }

    return (0);
}

/* Demana una peticio de registre de l'usuari amb adreca MI adrecaMI (cadena de
 * caràcters acabada en \0 i longitud <= 36) a traves del socket LUMI Sck.
 *
 * Escriu al fitxer de log amb identificador fitxerLog una linia informativa sobre
 * l'enviament de la petició de registre i, en cas de rebre una resposta, també
 * n'escriu una de nova en el mateix fitxer.
 *
 * Retorna -1 si l'enviament ha donat error, -2 si el format de adrecaMI no és
 * el correcte, -3 si el domini de adrecaMI no existeix; 0 si el registrament
 * s'ha fet correctament o 1 l'usuari de la adrecaMI no esta donat d'alta al
 * seu domini.
 */
int LUMIc_DemanaReg(int Sck, const char * adrecaMI, int fitxerLog) {
    char paquet[MIDA_PAQUET + 1];
    char domini[MIDA_PAQUET];

    // Obtenir domini
    if(obtenirDomini(adrecaMI,domini) == -1) {
        return (-2);
    }

    // Trobar la IP del node
    if(DNSc_ResolDNSaIP(domini,IP_NODE) == -1) {
        return (-3);
    }

    // Guardar l'adreca del node
    if(UDP_DemanaConnexio(Sck,IP_NODE,PORT_NODE) == -1) {
        return (-1);
    }

    // Crear paquet
    sprintf(paquet,"PR%s", adrecaMI);

    return enviarRegDesreg(Sck, paquet, fitxerLog);
}

/* Demana una peticio de desregistre de l'usuari amb adreca MI adrecaMI (cadena
 * de caracters acaba en \0 i longitud <= 37, incloent \0) a traves del socket LUMI amb
 * identificador Sck.
 *
 * Escriu al fitxer de log amb identificador fitxerLog una linia informativa sobre
 * l'enviament de la petició de registre i, en cas de rebre una resposta, també
 * n'escriu una de nova en el mateix fitxer.
 *
 * Retorna -1 si l'enviament ha donat error; 0 si el desregistre s'ha fet
 * correctament o 1 si l'usuari amb adrecaMI no existeix al seu domini.
 */
int LUMIc_DemanaDesreg(int Sck, const char * adrecaMI, const int fitxerLog) {
    char paquet[MIDA_PAQUET + 1];

    // Crear paquet
    sprintf(paquet,"PD%s", adrecaMI);

    return enviarRegDesreg(Sck, paquet, fitxerLog);
}

/* Demana una peticio de localitzacio de l'usuari amb adreca MI adrecaMI2 (cadena
 * de caracters acabada en \0 i longitud <= 37 incloent \0) per part de l'usuari amb adreca MI
 * adrecaMI1 (cadena de caracters acabada en \0 i longitud <= 37 incloent \0) a traves del
 * socket LUMI amb identificador Sck.
 *
 * En cas que la localitzacio es faci correctament, es a dir, es trobi l'usuari i
 * aquest pugui establir una conversa, omple IPremot (string de C de mida maxima 16)
 * i portremot amb la IP i el port de l'usuari amb adrecaMI2.
 *
 * Escriu al fitxer de log amb identificador fitxerLog una linia informativa sobre
 * l'enviament de la petició de registre i, en cas de rebre una resposta, també
 * n'escriu una de nova en el mateix fitxer.
 *
 * Retorna -1 si l'enviament de la peticio ha donat error; 0 si s'ha realitzat la
 * localitzacio correctament i l'usuari desti esta disponible per una conversa, 1
 * si l'usuari desti esta ocupat en una altra conversa, 2 si l'usuari desti o el
 * domini d'aquest no existeixen o 3 si l'usuari desti en aquests moments esta
 * offline
 */
int LUMIc_DemanaLoc(int Sck, const char * adrecaMI1, const char * adrecaMI2, char * IPremot, int * portremot, int fitxerLog) {
    char paquet[MIDA_PAQUET + 1];
    char paquetResposta[MIDA_PAQUET + 1];
    char codi;

    // Crear paquet
    sprintf(paquet,"PL%s:%s",adrecaMI1,adrecaMI2);

    // Enviar paquet
    if((enviarPaquet(Sck, paquet, paquetResposta, fitxerLog)) == -1)
        return (-1);

    // Tractar paquet segons el codi
    codi = paquetResposta[2];
    switch (codi) {
        case '0':
            if (obtenirIPport(paquetResposta, IPremot, portremot) == -1) {
                return (-1);
            }
            return(0);
        default:
            return paquetResposta[2] - '0';
    }
}

/* Respon una peticio de localitzacio a traves del socket LUMI amb identificador
 * Sck.
 *
 * El camp codi indica si l'usuari local pot acceptar la peticio de localitzacio,
 * per tant és 0, o ja esta ocupat en una conversa, i llavors codi es 1.
 *
 * Els camps IPMI (string de C de mida maxima 16) i portMI son la IP i el port
 * d'escolta de l'usuari local.
 *
 * Escriu al fitxer de log amb identificador fitxerLog una linia informativa sobre
 * la peticio de localitzacio rebuda i a continuacio, una nova linia amb la informacio
 * de la resposta a la peticio.
 *
 * Retorna -1 si no s'ha pogut respondre a la localitzacio; 0 si tot ha anat correctament.
 */
int LUMIc_RespondreLoc(int Sck, int codi, const char * IPMI, const int * portMI, int fitxerLog) {
    char paquet[MIDA_PAQUET + 1];
    char paquetResposta[MIDA_PAQUET + 1];
    int bytesRebuts;
    int bytesEnviats;

    // Rebre la peticio localitzacio
    if((bytesRebuts = UDP_Rep(Sck,paquet,MIDA_PAQUET)) == -1) {
        return (-1);
    }
    paquet[bytesRebuts] = '\0';
    if(escriureLog(fitxerLog,'R',paquet,bytesRebuts) == -1) {
        return (-1);
    }

    // Obtenir les adreces MI
    char adreces[bytesRebuts+1-2];
    memcpy(adreces, &paquet[2],sizeof(adreces));

    // Crear paquet de resposta
    if(codi == 0) sprintf(paquetResposta,"RL%d%s:%s:%d",codi, adreces, IPMI, *portMI);
    else sprintf(paquetResposta,"RL%d%s",codi, adreces);

    // Enviar peticio
    if((bytesEnviats = UDP_Envia(Sck,paquetResposta,strlen(paquetResposta))) == -1) {
        return (-1);
    }
    if(escriureLog(fitxerLog,'E',paquetResposta,bytesEnviats) == -1) {
        return (-1);
    }

    return (0);
}

/* Obté un missatge de text que descriu l'error produït en la darrera     */
/* crida de sockets UDP (de la part client de LUMI).                      */
/*                                                                        */
/* Retorna aquest missatge de text en un "string" de C (vector de chars   */
/* imprimibles acabat en '\0')                                            */
/*                                                                        */
/* (aquesta funció és equivalent a la funció UDP_ObteMissError() de la    */
/* "nova" interfície de la capa UDP però ara a la capa d’aplicació)       */
char* LUMIc_ObteMissError(void)
{
    return UDP_ObteMissError();
}

/* * * * * * * * * * * * * * * * * * * * * * * * * */
/*              FUNCIONS INTERNES                  */
/* * * * * * * * * * * * * * * * * * * * * * * * * */

/* Envia un paquet de peticio registre o peticio desregistre a traves
 * del socket LUMI amb identificador Sck.
 *
 * El paquet es una cadena de caracters acabada en \0 i de longitud
 * MIDA_PAQUET+1 , incloent el\0.
 *
 * Escriu al fitxer de log amb identificador fitxerLog una linia informativa sobre
 * l'enviament de la petició i, en cas de rebre una resposta, també
 * n'escriu una de nova en el mateix fitxer.
 *
 * Retorna -1 si hi ha error en l'enviament, 0 si el registre o
 * desregistre s'ha fet correctament o 1 si l'usuari no existeix en
 * el domini del servidor.
 */
int enviarRegDesreg(int Sck, const char * paquet, int fitxerLog) {
    char paquetResposta[MIDA_PAQUET + 1];

    if((enviarPaquet(Sck, paquet, paquetResposta, fitxerLog)) == -1)
        return (-1);

    //Retorna la respota a la petició
    return paquetResposta[strlen(paquetResposta)-1] == '1' ? 1 : 0;
}

/* Envia un paquet (cadena de caracters acabada en \0 i de longitud maxima MIDA_PAQUET+1,
 * incloent \0) a traves del socket LUMI amb identificador Sck.
 *
 * Omple paquetResposta (string de C de mida màxima MIDA_PAQUET+1, incloent \0) amb la
 * resposta rebuda a traves del socket LUMI a causa del paquet que s'ha enviat.
 *
 * Escriu al fitxer de log amb identificador fitxerLog una linia informativa sobre
 * l'enviament de la petició i, en cas de rebre una resposta, també
 * n'escriu una de nova en el mateix fitxer.
 *
 * Retorna -1 si hi hagut un error; 0 si tot ha anat correctament.
 */
int enviarPaquet(int Sck, const char * paquet, char * paquetResposta, int fitxerLog) {
    int bytesEnviats;
    int bytesRebuts;
    int calAcabar = 0;
    int intents = 0;
    char resposta[MIDA_PAQUET + 1];

    // Intentar enviar el paquet un nombre determinat de vegades
    while (calAcabar == 0 && intents < MAX_ATTEMPTS) {
        // Enviar peticio
        if((bytesEnviats = UDP_Envia(Sck,paquet,strlen(paquet))) != -1) {
            // Escriure en el log l'enviament
            if (escriureLog(fitxerLog, 'E', paquet, bytesEnviats) == -1) {
                return (-1);
            }

            // Esperar i tractar resposta
            int llista[1] = {Sck};
            int aux = T_HaArribatAlgunaCosaEnTemps(llista, 1, TIMEOUT);
            if (aux == -1) {
                return (-1);
            }
            if (aux != -2) calAcabar = 1;
            else intents++;
        }
        else intents++;
    }
    if(intents == MAX_ATTEMPTS) {
        errno = ECOMM; //error de comunicacio al enviar
        return (-1);
    }

    // Rebre resposta a la peticio enviada
    if((bytesRebuts = UDP_Rep(Sck,resposta,MIDA_PAQUET)) == -1) {
        return (-1);
    }
    resposta[bytesRebuts] = '\0';
    if(escriureLog(fitxerLog,'R',resposta,bytesRebuts) == -1) {
        return (-1);
    }
    strcpy(paquetResposta,resposta);

    return (0);
}

/*
 * Omple domini (cadena de caracters acabada amb \0 i de mida <= 34) amb
 * el domini que hi figura a adrecaMI.
 *
 * AdrecaMI es un string de C, de mida màxima 36 caracters i de la forma
 * nom@domini.
 *
 * Retorna -1 si el format de l'adrecaMI no es correcte; 0 si tot ha
 * anat be.
 */
int obtenirDomini(const char * adrecaMI, char * domini) {
    char * token;
    char aux[strlen(adrecaMI)+1];
    strcpy(aux,adrecaMI);

    if((token = strtok(aux, "@")) == NULL){
        return (-1);
    }
    if((token = strtok(NULL, "@")) == NULL){
        return (-1);
    }
    strcpy(domini, token);
    return (0);
}

/*
 * Omple IP (string de C de mida maxima 16 caracters) i port amb la IP i el port remot
 * que apareixen en el paquet (string de C de mida MIDA_PAQUET+1) passat com a parametre.
 *
 * El paquet es una resposta de localitzacio amb tipus 0.
 *
 * Retorna -1 si el format del paquet no és correcte; 0 si tot ha anat be.
 */
int obtenirIPport(const char * paquet, char * IP, int * port) {
    char * token;
    char aux[strlen(paquet)+1];
    strcpy(aux,paquet);

    if((token = strtok(aux, ":")) == NULL){
        return (-1);
    }
    if((token = strtok(NULL, ":")) == NULL){
        return (-1);
    }
    if((token = strtok(NULL, ":")) == NULL){
        return (-1);
    }
    strcpy(IP, token);
    if((token = strtok(NULL, ":")) == NULL){
        return (-1);
    }
    *port = strtol(token,NULL,10);

    return (0);
}

// retorna 0 si s'ha escrit correctament, -1 si no s'ha pogut

/* Escriu en el fitxer de log amb identificador fitxerLog una linia informativa
 * indicant si el paquet (cadena de caracters acabada en \0 de mida maxima
 * MIDA_PAQUET+1, incloent \0) s'ha enviat o rebut, la IP i el port del remot, el paquet
 * i el total de bytes enviats o rebuts.
 *
 * El camp enviatRebut es E si s'ha enviat el paquet o R si s'ha rebut.
 *
 * Retorna -1 si hi ha hagut un error; 0 si tot ha anat bé.
 */
int escriureLog(int fitxerLog, const char enviatRebut, const char * paquet, int bytes) {
    char buffer[200];
    sprintf(buffer,"%c: %s:UDP:%d, %s, %d",enviatRebut, IP_NODE, PORT_NODE, paquet, bytes);

    if(Log_Escriu(fitxerLog, buffer) == -1){
        return (-1);
    }
    return (0);
}

/* Definició de funcions INTERNES, és a dir, d'aquelles que es faran      */
/* servir només en aquest mateix fitxer. Les seves declaracions es        */
/* troben a l'inici d'aquest fitxer.                                      */

/* Crea un fitxer de "log" de nom "NomFitxLog".                           */
/*                                                                        */
/* "NomFitxLog" és un "string" de C (vector de chars imprimibles acabat   */
/* en '\0') d'una longitud qualsevol.                                     */
/*                                                                        */
/* Retorna -1 si hi ha error; l'identificador del fitxer creat si tot va  */
/* bé.                                                                    */
int Log_CreaFitx(const char *NomFitxLog)
{
 int fd;

 /* Amb O_APPEND: si existeix el fitxer que es vol crear, s'escriu a */
 /* continuació mantenint el que hi havia; amb  O_TRUNC, si existeix */
 /* el fitxer que es vol crear, es trunca  a 0 bytes (s'esborra).    */
 fd = open(NomFitxLog, O_CREAT | O_WRONLY | O_APPEND | O_SYNC, 00644);
 if (fd==-1) return -1;
 
 if(write(fd, "------ INICI LOG ------\n", 24 )==-1) return -1;
 
 return fd; 
}

/* Escriu al fitxer de "log" d'identificador "FitxLog" el missatge de     */
/* "log" "MissLog".                                                       */
/*                                                                        */
/* "MissLog" és un "string" de C (vector de chars imprimibles acabat      */
/* en '\0') d'una longitud qualsevol.                                     */
/*                                                                        */
/* Retorna -1 si hi ha error; el nombre de caràcters del missatge de      */
/* "log" (sense el '\0') si tot va bé.                                    */
int Log_Escriu(int FitxLog, const char *MissLog)
{
 int ncars;

 if ((ncars = write(FitxLog, MissLog, strlen(MissLog)))==-1) return -1;

 char aux[] = "\n";
 if (write(FitxLog, aux, strlen(aux))==-1) return -1;

 return ncars;
}

/* Tanca el fitxer de "log" d'identificador "FitxLog".                    */
/*                                                                        */
/* Retorna -1 si hi ha error; 0 si tot va bé.                             */
int Log_TancaFitx(int FitxLog)
{
 if (write(FitxLog, "------ FINAL LOG ------\n", 24 )==-1) return -1;
 
 return close(FitxLog);
}

/* Si ho creieu convenient, feu altres funcions INTERNES                  */

/* Descripció de la funció, dels arguments, valors de retorn, etc.        */
/* int FuncioInterna(arg1, arg2...)
{
	
} */
