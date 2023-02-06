/**************************************************************************/
/*                                                                        */
/* P2 - MI amb sockets TCP/IP - Part II                                   */
/* Fitxer aLUMIs.c que implementa la capa d'aplicació de MI, sobre        */
/* la capa de transport UDP (fent crides a la "nova" interfície de la     */
/* capa UDP o "nova" interfície de sockets), en la part servidora.        */
/* Autors: Laura Galera Alfaro i David Perez Sanchez                      */
/*                                                                        */
/**************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "MIp2-tUDP.h"
#include "MIp2-aDNSc.h"
#include "MIp2-aLUMIs.h"

const char PATH_CONFIG[30] = "MIp2-nodelumi.cfg";
const char IP_NODE[16] = "0.0.0.0";
const int PORT_NODE = 3344;
const int MIDA_PAQUET = 100;
const int MAX_ATTEMPTS = 5;
char DominiServidor[100];

/*****************************************************/
/**        Declaracio de funcions internes          **/
/*****************************************************/

/* Servir peticions de cada tipus */
int tractarPeticioRegistrar(int Sck, Registre * reg, const char * adrecaMI, const char * IPlumi, int portlumi, int fitxerLog);
int tractarPeticioDesregistrar(int Sck, Registre * reg, const char * adrecaMI, const char * IPlumi, int portlumi, int fitxerLog);
int tractarPeticioLocalitzacio(int Sck, Registre * reg, const char * paquet, const char * IPant, int portant, int fitxerLog);
int tractarRespostaLocalitzacio(int Sck, Registre * reg, const char * paquet, int fitxerLog);

/* Auxiliars per servir peticions */
int buscarPosRegistre(const Registre * reg, const char * nom);
int enviarPaquet(int Sck, const char * IPremot, int portremot, const char * paquet, int fitxerLog);
int obtenirNomDomini(const char * adrecaMI, char * nomMI, char * domini);

/* Funcions del fitxer Log */
int escriureLog(int fitxerLog, const char enviatRebut, const char * paquet, const char * IPremot, int portremot, int bytes);
int Log_CreaFitx(const char *NomFitxLog);
int Log_Escriu(int FitxLog, const char *MissLog);
int Log_TancaFitx(int FitxLog);

/* Escriure el fitxer de config */
int escriureRegistre(const Registre * reg);

/*****************************************************/
/**       Implementacio de funcions externes        **/
/*****************************************************/

/* Inicia el socket LUMI a través d'un nou socket UDP, amb IP qualsevol
 * i port 3344 i guarda l'indentificador del socket a Sck.
 *
 * Crea un fitxer de log amb nom "nodelumi-domini.log" i guarda el nou
 * identificador del fitxer a fitxerLog.
 *
 * Llegeix el fitxer de configuracio i carrega a domini el domini del servidor
 * i a la taula de registres reg tots els usuaris que apareixen al fitxer,
 * inicialitzats a offline.
 *
 * Retorna -1 si hi hagut un error; 0 si tot ha anat be.
 *
 */
int LUMIs_IniciaServidor(int * Sck, Registre * reg, int * fitxerLog, char * domini) {
    int i;

    // Llegir fitxer de configuracio
    FILE * fileReg;
    if((fileReg = fopen(PATH_CONFIG,"r")) == NULL){
        return (-1);
    }
    if(fscanf(fileReg, "%s", DominiServidor) == EOF) {
        return(-1);
    }
    i = (*reg).nElem = 0;
    while(fscanf(fileReg, "%s", (*reg).taula[i].adrecaMI) != EOF && i < MAX_REG) {
        (*reg).taula[i].online = 0;
        (*reg).nElem++;
        i++;
    }
    fclose(fileReg);

    // Crear socket LUMI
    if((*Sck = UDP_CreaSock(IP_NODE, PORT_NODE)) == -1){
        return (-1);
    }

    // Crear fitxer de Log
    char buff[200];
    sprintf(buff, "nodelumi-%s.log",DominiServidor);
    if((*fitxerLog = Log_CreaFitx(buff)) == -1) {
        return (-1);
    }

    strcpy(domini,DominiServidor);

    return (0);
}

/*
 * Sobreescriu els usuaris de la taula de registres reg
 * al fitxer de configuracio.
 *
 * Tanca el socket LUMI amb identificador Sck i el fitxer de
 * log amb identificador fitxerLog.
 *
 * Retorna -1 si hi ha un error; 0 si tot va be.
 */
int LUMIs_FinalitzaServidor(int Sck, int fitxerLog, const Registre * reg) {
    if(UDP_TancaSock(Sck) == -1) {
        return (-1);
    }
    if(Log_TancaFitx(fitxerLog) == -1) {
        return (-1);
    }

    return escriureRegistre(reg);
}

/*
 * Rep un paquet a partir del socket LUMI amb identificador Sck i
 * el tracta segons el tipus de paquet: peticio de localitzacio,
 * peticio de registre, peticio de desregistre i resposta de
 * localitzacio.
 *
 * Escriu al fitxer de log amb identificador fitxerLog una linia informativa sobre
 * el paquet rebut i, en cas d'enviament, també n'escriu una de nova en el mateix
 * fitxer.
 *
 * Segons el tipus de paquet rebut, modifica la taula de registres reg per
 * tal de satisfer la solicitud rebuda.
 *
 * Retorna -1 si hi ha hagut un error; 0 si tot ha anat be.
 */
int LUMIs_ServeixPeticio(int Sck, Registre * reg, int fitxerLog){
    char IPrem[16];
    int portrem;
    char paquet[MIDA_PAQUET + 1];
    int bytesRebuts;

    // Rep una peticio
    if((bytesRebuts = UDP_RepDe(Sck, IPrem, &portrem, paquet, MIDA_PAQUET)) == -1) {
        return(-1);
    }
    paquet[bytesRebuts] = '\0';
    if(escriureLog(fitxerLog,'R',paquet, IPrem, portrem, bytesRebuts) == -1) {
        return (-1);
    }

    // Agafa el tipus
    char tipus[3];
    memcpy(tipus,&paquet[0],2);
    tipus[sizeof(tipus)-1] = '\0';

    // Peticio de localitzacio
    if(strcmp(tipus,"PL") == 0) {
        if(tractarPeticioLocalitzacio(Sck, reg, paquet, IPrem, portrem, fitxerLog) == -1)
            return (-1);
    }
    // Resposta de localitzacio
    else if(strcmp(tipus,"RL") == 0){
        if(tractarRespostaLocalitzacio(Sck, reg, paquet, fitxerLog) == -1)
            return (-1);
    }
    // Registre/Desregistre
    else {
        // Agafa la adreçaMI
        char info[bytesRebuts-1];
        memcpy(info,&paquet[2],bytesRebuts-2);
        info[sizeof(info)-1] = '\0';

        // Peticio de registrar
        if(strcmp(tipus,"PR") == 0){
            if(tractarPeticioRegistrar(Sck, reg, info, IPrem, portrem, fitxerLog) == -1)
                return (-1);
        }
        // Peticio de desregistrar
        else if(strcmp(tipus,"PD") == 0) {
            if(tractarPeticioDesregistrar(Sck, reg, info, IPrem, portrem, fitxerLog) == -1)
                return (-1);
        }
        // Peticio desconeguda
        else return (-1);
    }
    return (0);
}

/* Obté un missatge de text que descriu l'error produït en la darrera     */
/* crida de sockets UDP (de la part servidora de LUMI).                   */
/*                                                                        */
/* Retorna aquest missatge de text en un "string" de C (vector de chars   */
/* imprimibles acabat en '\0')                                            */
/*                                                                        */
/* (aquesta funció és equivalent a la funció UDP_ObteMissError() de la    */
/* "nova" interfície de la capa UDP però ara a la capa d’aplicació)       */
char* LUMIs_ObteMissError(void)
{
    return UDP_ObteMissError();
}

/*****************************************************/
/**       Implementacio de funcions internes        **/
/*****************************************************/

/*
 * Tracta la peticio de registre d'un usuari amb adreca adrecaMI (cadena
 * de caracters acabada amb \0 i de mida <= 37, incloent \0) al servidor d'aquest domini,
 * que coincideix amb el domini que figura a adrecaMI i envia una resposta
 * de registre a traves del socket Sck a la IP i port que hi ha a IPlumi i portlumi.
 *
 * Escriu al fitxer de log amb identificador fitxerLog una linia informativa sobre
 * la peticio de registre i, en cas d'enviar una resposta, també n'escriu una de nova
 * en el mateix fitxer.
 *
 * A la taula de registre reg es posa a l'usuari sollicitat en linia i es guarden
 * la seva IP i port de socket LUMI.
 *
 * Retorna -1 si hi ha un error; 0 si tot ha anat be.
 */
int tractarPeticioRegistrar(int Sck, Registre * reg, const char * adrecaMI, const char * IPlumi, int portlumi, int fitxerLog) {
    char nomMI[MIDA_PAQUET];
    char paquet[MIDA_PAQUET + 1];

    // Obtenir nomMI
    if(obtenirNomDomini(adrecaMI, nomMI, NULL) == -1) {
        return (-1);
    }

    // Tractar registre
    int pos = buscarPosRegistre(reg, nomMI);
    if(pos != -1) {
        (*reg).taula[pos].online = 1;
        (*reg).taula[pos].intents = 0;
        strcpy((*reg).taula[pos].adrecaLUMI.IP, IPlumi);
        (*reg).taula[pos].adrecaLUMI.port = portlumi;
        // Enviar paquet de resposta de registre correcte
        sprintf(paquet,"RR0");
        if(enviarPaquet(Sck, IPlumi, portlumi, paquet, fitxerLog) == -1)
            return (-1);
    }
    else {
        // Enviar paquet de resposta amb error
        sprintf(paquet,"RR1");
        if (enviarPaquet(Sck, IPlumi, portlumi, paquet, fitxerLog) == -1)
            return (-1);
    }

    return (0);
}

/*
 * Tracta la peticio de desregistre d'un usuari amb adreca adrecaMI (cadena de caracters
 * acabada amb \0 i de mida <= 37, incloent \0) al servidor d'aquest domini, que coincideix amb el
 * domini que figura a adrecaMI. I envia una resposta de desregistre a traves del socket
 * Sck a la IP i port que hi ha a IPlumi i portlumi.
 *
 * Escriu al fitxer de log amb identificador fitxerLog una linia informativa sobre
 * la peticio de desregistre i, en cas d'enviar una resposta, també n'escriu una de nova
 * en el mateix fitxer.
 *
 * A la taula de registre reg es posa a l'usuari sollicitat com a fora de linia.
 *
 * Retorna -1 si hi ha un error; 0 si tot ha anat be.
 */
int tractarPeticioDesregistrar(const int Sck, Registre * reg, const char * adrecaMI, const char * IPlumi,  const int portlumi, const int fitxerLog){
    char nomMI[MIDA_PAQUET];
    char paquet[MIDA_PAQUET + 1];

    // Obtenir nomMI
    if(obtenirNomDomini(adrecaMI, nomMI, NULL) == -1) {
        return (-1);
    }

    // Tractar desregistre
    int pos = buscarPosRegistre(reg, nomMI);
    if(pos != -1) {
        (*reg).taula[pos].online = 0;
        // Enviar paquet de resposta desregistre correcte
        sprintf(paquet,"RD0");
        if(enviarPaquet(Sck, IPlumi, portlumi, paquet, fitxerLog) == -1)
            return (-1);
    }
    else {
        // Enviar paquet de de resposta desregistre incorrecte
        sprintf(paquet,"RD1");
        if(enviarPaquet(Sck, IPlumi, portlumi, paquet, fitxerLog) == -1)
            return (-1);
    }
    return (0);
}

/*
 * Tracta la peticio de localitzacio que apareix a paquet (cadena de caracters
 * acabada en \0 i de mida <= MIDA_PAQUET+1, incloent \0) d'un usuari a un
 * altre usuari. Reenvia a traves del socket LUMI amb identificador Sck
 * el paquet al domini que toca o directament al client per tal de satisfer la
 * peticio. En cas que la localitzacio no sigui possible, envia a traves de Sck
 * a la IP IPant i al port portant una resposta de localitzacio.
 *
 * IPant (cadena de caracters de mida 16) i portant son la IP i el port des dels
 * quals s'ha rebut la peticio de localitzacio.
 *
 * Escriu al fitxer de log amb identificador fitxerLog una linia informativa sobre
 * la peticio de localitzacio i, en cas de reenviar-la o enviar una resposta de
 * localitzacio, també n'escriu una de nova en el mateix fitxer.
 *
 * La taula de registres reg es la taula amb tots els usuaris donats d'alta en
 * aquest node. En cas que l'usuari pertanyi aquest domini i no respongui a la
 * peticio al cap de MAX_ATTEMPTS se'l considerara fora de linia.
 *
 * Retorna -1 si hi ha un error; 0 si tot ha anat be.
 */
int tractarPeticioLocalitzacio(int Sck, Registre * reg, const char * paquet, const char * IPant, int portant, int fitxerLog) {
    char adrecaMI1[MIDA_PAQUET];
    char adrecaMI2[MIDA_PAQUET];
    char domini[MIDA_PAQUET];
    char nomMI[MIDA_PAQUET];
    char paquetResposta[MIDA_PAQUET + 1];
    char * token;

    // Obtenir adreces MI
    char aux[strlen(paquet)+1];
    strcpy(aux,paquet);
    if((token = strtok(aux, ":")) == NULL)
        return (-1);
    strcpy(adrecaMI1,token);
    if((token = strtok(NULL, ":")) == NULL)
        return (-1);
    strcpy(adrecaMI2,token);

    // Treure el tipusPaquet i convertir a string
    int longitud = strlen(adrecaMI1)-2;
    memcpy(adrecaMI1,&adrecaMI1[2],longitud);
    adrecaMI1[longitud] = '\0';

    // Separa el nomMI i el domini
    if(obtenirNomDomini(adrecaMI2, nomMI, domini) == -1) {
        return (-1);
    }
    // Tractar peticio localitzacio
    if(strcmp(DominiServidor, domini) == 0) { // El domini de l'usuari desti coincideix
        // Buscar usuari desti a la taula
        int pos = buscarPosRegistre(reg, nomMI);

        if(pos != -1) { // Si esta donat d'alta
            if((*reg).taula[pos].online == 1) { // Si esta online

                // Sumar un intent
                (*reg).taula[pos].intents++;

                // Si s'ha arribat al màxim d'intents
                if((*reg).taula[pos].intents == MAX_ATTEMPTS) {
                    (*reg).taula[pos].online = 0;
                    // Enviar resposta a l'estacio anterior
                    sprintf(paquetResposta,"RL3%s:%s", adrecaMI1, adrecaMI2);
                    if(enviarPaquet(Sck, IPant, portant, paquetResposta, fitxerLog) == -1)
                        return (-1);
                }
                else {
                    // Preguntar al client el seu socket MI
                    int portlumi = (*reg).taula[pos].adrecaLUMI.port;
                    char IPlumi[16];
                    strcpy(IPlumi, (*reg).taula[pos].adrecaLUMI.IP);
                    if (enviarPaquet(Sck, IPlumi, portlumi, paquet, fitxerLog) == -1) {
                        return (-1);
                    }
                }
            }
            else { // Si esta offline
                // Enviar resposta a l'estacio anterior
                sprintf(paquetResposta,"RL3%s:%s", adrecaMI1, adrecaMI2);
                if(enviarPaquet(Sck, IPant, portant, paquetResposta, fitxerLog) == -1)
                    return (-1);
            }
        }
        else { // Usuari no donat d'alta
            // Enviar resposta a l'estacio anterior
            sprintf(paquetResposta,"RL2%s:%s", adrecaMI1, adrecaMI2);
            if(enviarPaquet(Sck, IPant, portant, paquetResposta, fitxerLog) == -1)
                return (-1);
        }
    }
    else { // El domini no coincideix amb el d'aquest node
        // Trobar la IP de l'altre node
        char IPservidorVei[16];
        if (DNSc_ResolDNSaIP(domini, IPservidorVei) == -1) { // No s'ha trobat domini
            // Enviar resposta a l'estacio anterior
            sprintf(paquetResposta,"RL2%s:%s", adrecaMI1, adrecaMI2);
            if(enviarPaquet(Sck, IPant, portant, paquetResposta, fitxerLog) == -1)
                return (-1);
        }
        else { // Existeix el domini del desti
            // Enviar peticio al node desti
            if(enviarPaquet(Sck, IPservidorVei, PORT_NODE, paquet, fitxerLog) == -1)
                return (-1);
        }
    }
    return (0);
}

/*
 * Tracta la resposta de localitzacio que apareix a paquet (cadena de caracters
 * acabada en \0 i de mida <= MIDA_PAQUET+1, incloent \0) d'un usuari o node a un
 * altre usuari. Reenvia a traves del socket LUMI amb identificador Sck
 * el paquet al domini que toca o directament al client si pertany a aquest node.
 *
 * Escriu al fitxer de log amb identificador fitxerLog una linia informativa sobre
 * la resposta de localitzacio i, en cas de reenviar-la, també n'escriu una de
 * nova en el mateix fitxer.
 *
 * La taula de registres reg es la taula amb tots els usuaris donats d'alta en
 * aquest node.
 *
 * Retorna -1 si hi ha un error; 0 si tot ha anat be.
 */
int tractarRespostaLocalitzacio(int Sck, Registre * reg, const char * paquet, int fitxerLog) {
    char adrecaMI1[MIDA_PAQUET];
    char adrecaMI2[MIDA_PAQUET];
    char domini[MIDA_PAQUET];
    char nomMI[MIDA_PAQUET];
    char * token;

    // Obtenir adreca del client origen
    char aux[strlen(paquet)+1];
    strcpy(aux,paquet);
    if((token = strtok(aux, ":")) == NULL)
        return (-1);
    strcpy(adrecaMI1,token);
    // Treure el tipusPaquet i convertir a string
    int longitud = strlen(adrecaMI1)-3;
    memcpy(adrecaMI1,&adrecaMI1[3],longitud);
    adrecaMI1[longitud] = '\0';

    // Obtenir adreca client desti
    if((token = strtok(NULL, ":")) == NULL) {
        return (-1);
    }
    strcpy(adrecaMI2,token);

    // Separa el nomMI i el domini de origen
    if(obtenirNomDomini(adrecaMI1, nomMI, domini) == -1) {
        return (-1);
    }

    if(strcmp(domini, DominiServidor) == 0) { // El domini origen coincideix amb el domini d'aquest node
        // Passar paquet al client origen
        int pos;
        if((pos = buscarPosRegistre(reg, nomMI)) == -1)
            return (-1);
        int portlumi = (*reg).taula[pos].adrecaLUMI.port;
        char IPlumi[16];
        strcpy(IPlumi,(*reg).taula[pos].adrecaLUMI.IP);
        if(enviarPaquet(Sck, IPlumi, portlumi, paquet, fitxerLog) == -1)
            return (-1);
    }
    else { // El domini origen no coincideix
        // Passar paquet al node origen
        char IPservidorVei[16];
        if (DNSc_ResolDNSaIP(domini, IPservidorVei) == -1)
            return (-1);
        if(enviarPaquet(Sck, IPservidorVei, PORT_NODE, paquet, fitxerLog) == -1)
            return (-1);
    }

    // Separa el nomMI i el domini
    if(obtenirNomDomini(adrecaMI2, nomMI, domini) == -1) {
        return (-1);
    }

    // Inicialitzar intents
    if(strcmp(domini, DominiServidor) == 0) { //segur es RL0 o RL1, pk sino ho envia el node
        int pos;
        if((pos = buscarPosRegistre(reg, nomMI)) == -1) {
            return (-1);
        }
        (*reg).taula[pos].intents = 0;
    }
    return (0);
}

/*
 * Omple els camps nomMI (cadena de caracters acabada en \0 i de mida <= 35)
 * i domini (cadena de caracters acabada en \0 i de mida <= 35) amb el nom
 * i el domini que apareixen a adrecaMI ((cadena de caracters acabada en \0
 * i de mida <= 36, incloent \0).
 *
 * Retorna -1 si el format de adrecaMI no es nomMI@domini; 0 si tot va be.
 *
 */
int obtenirNomDomini(const char * adrecaMI, char * nomMI, char * domini){
    char * token;
    char aux[strlen(adrecaMI)+1];
    strcpy(aux,adrecaMI);

    if((token = strtok(aux, "@")) == NULL){
        return (-1);
    }
    if(nomMI != NULL) strcpy(nomMI, token);
    if((token = strtok(NULL, "@")) == NULL){
        return (-1);
    }
    if(domini != NULL) strcpy(domini, token);
    return (0);
}

/* Busca a la taula de registres reg, l'usuari amb nomMI nom (cadena
 * de caracters acabada en \0 i de mida <= 35, incloent \0)
 *
 * Retorna -1 si no s'ha trobat; la posicio dins de la taula
 * de registres en cas que existeixi
 */
int buscarPosRegistre(const Registre * reg, const char * nom){
    int trobat = 0;
    int i = 0;
    int res = -1;
    while(i < (*reg).nElem && trobat == 0) {
        if(strcmp((*reg).taula[i].adrecaMI, nom) == 0) {
            res = i;
            trobat = 1;
        }
        else i++;
    }
    return res;
}

/* Envia un paquet (cadena de caracters acabada en \0 i de longitud maxima MIDA_PAQUET+1,
 * incloent \0) a traves del socket LUMI, amb identificador Sck, a la IP i port que figuren
 * a IPremot (cadena de caracters de mida 16, incloent \0) i portremot.
 *
 * Escriu al fitxer de log amb identificador fitxerLog una linia informativa sobre
 * l'enviament del paquet.
 *
 * Retorna -1 si hi hagut un error; 0 si tot ha anat correctament.
 */
int enviarPaquet(int Sck, const char * IPremot, int portremot, const char * paquet, int fitxerLog) {
    int bytesEnviats;

    if((bytesEnviats = UDP_EnviaA(Sck, IPremot, portremot, paquet, strlen(paquet))) == -1) {
        return (-1);
    }
    if(escriureLog(fitxerLog,'E',paquet, IPremot, portremot, bytesEnviats) == -1) {
        return (-1);
    }
    return (0);
}

/* Escriu en el fitxer de log amb identificador fitxerLog una linia informativa
 * indicant si el paquet (cadena de caracters acabada en \0 de mida maxima
 * MIDA_PAQUET+1, incloent \0) s'ha enviat o rebut, la IP i el port del remot, el paquet
 * i el total de bytes enviats o rebuts.
 *
 * El camp enviatRebut es E si s'ha enviat el paquet o R si s'ha rebut.
 *
 * IPremot (cadena de caracters de mida 16) i el portremot son la IP i el port des dels
 * quals s'ha rebut o enviat el paquet.
 *
 * Retorna -1 si hi ha hagut un error; 0 si tot ha anat bé.
 */
int escriureLog(int fitxerLog, const char enviatRebut, const char * paquet, const char * IPremot, int portremot, int bytes) {
    char buffer[200];
    sprintf(buffer,"%c: %s:UDP:%d, %s, %d",enviatRebut,IPremot,portremot,paquet,bytes);
    if(Log_Escriu(fitxerLog, buffer) == -1){
        return (-1);
    }
    return (0);
}

/*
 * Sobreescriu el que hi ha al fitxer de config amb el nom del domini
 * del servidor i tots els usuaris donats d'alta en el node que apareixen
 * a la taula de registres reg.
 *
 * Retorna -1 si hi ha error; 0 si tot va be.
 */
int escriureRegistre(const Registre * reg) {
    int i;
    FILE * fileReg;
    if((fileReg = fopen(PATH_CONFIG,"w")) == NULL){
        return (-1);
    }

    fprintf(fileReg,"%s\n",DominiServidor);

    for(i = 0; i < reg->nElem; i++) {
        fprintf(fileReg,"%s\n",reg->taula[i].adrecaMI);
    }

    fclose(fileReg);

    return (0);
}

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
