/**************************************************************************/
/*                                                                        */
/* P2 - MI amb sockets TCP/IP - Part II                                   */
/* Fitxer MIp2-nodelumi.c que implementa la interfície aplicació-         */
/* administrador d'un node de LUMI, sobre la capa d'aplicació de (la      */
/* part servidora de) LUMI (fent crides a la interfície de la part        */
/* servidora de LUMI).                                                    */
/* Autors: Laura Galera Alfaro i David Perez Sanchez                      */
/*                                                                        */
/**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "MIp2-aLUMIs.h"
#include "MIp2-aA.h"

// Colors per la interficie
#define BOLDWHITE   "\033[1m\033[37m"
#define BLAU  "\x1B[36m"
#define VERD  "\x1B[32m"
#define VERMELL  "\x1B[31m"
#define NORMAL  "\x1B[0m"

// Accions
void inicialitzar(int * sckLumi, Registre * reg, int * fitxerLog);
void finalitzar(int sckLumi, int fitxerLog, const Registre * reg);
void mostrarMenu();
void mostrarRegistre(const Registre * reg);
void mostraSeparador();

// Funcions
int afegirUsuari(Registre * reg, const char * nom);
int treureUsuari(Registre * reg, const char * nom);
int buscarEnRegistre(const Registre * reg, const char * nom);

int main(int argc,char *argv[])
{
    Registre reg;
    int sckLumi;
    int fitxerLog;
    int llistaFD[2];
    int calAcabar = 0;

    inicialitzar(&sckLumi, &reg, &fitxerLog);

    llistaFD[0] = 0;
    llistaFD[1] = sckLumi;

    mostraSeparador();
    printf("Escriu %s/help%s per mostrar l'ajuda\n",BLAU,NORMAL);
    mostraSeparador();

    // Mostrar prompt
    printf("%s> %s",BOLDWHITE,NORMAL);
    fflush(stdout);

    do {
        int fd;
        if ((fd = A_HaArribatAlgunaCosa(llistaFD, sizeof(llistaFD) / sizeof(int))) == -1) {
            printf("%sError mirant si ha arribat alguna cosa: %s%s\n", VERMELL, A_ObteMissError(), NORMAL);
            exit(-1);
        }

        // Servir peticions
        if (fd == sckLumi) {
            if (LUMIs_ServeixPeticio(sckLumi, &reg, fitxerLog) == -1) {
                printf("%sError servint petició: %s%s\n", VERMELL, LUMIs_ObteMissError(), NORMAL);
                exit(-1);
            }
        }

        // Gestionar el node
        if (fd == 0) {
            char buffer[200];
            char * token;
            int bytes_llegits;

            if((bytes_llegits=read(0,buffer,sizeof(buffer)-1))==-1){
                printf("%sError en el read%s\n", VERMELL, NORMAL);
                exit (-1);
            }
            buffer[bytes_llegits-1] = '\0';

            if((token = strtok(buffer, " ")) == NULL){
                printf("%sFormat incorrecte%s\n",VERMELL,NORMAL);
            }

            if(strcmp("/help",token) == 0) {
                mostrarMenu();
            }
            else if(strcmp("/exit",token) == 0) {
                calAcabar = 1;
            }
            else if(strcmp("/llistar",token) == 0) {
                mostrarRegistre(&reg);
            }
            else {
                int esAlta = strcmp("/alta", token) == 0;
                int esBaixa = strcmp("/baixa", token) == 0;
                if (esAlta == 1 || esBaixa == 1) {
                    if((token = strtok(NULL, " ")) == NULL){
                        printf("%sFormat incorrecte%s\n",VERMELL,NORMAL);
                    }
                    else {
                        if (esAlta == 1) {
                            int aux = afegirUsuari(&reg, token);
                            if (aux == -1) printf("%sNo es pot afegir: Registre ple%s\n",VERMELL,NORMAL);
                            else if(aux == -2) printf("%sNo es pot afegir: Usuari ja existent%s\n",VERMELL,NORMAL);
                            else printf("Usuari %s%s%s afegit\n", VERD,token,NORMAL);
                        }
                        else {
                            if (treureUsuari(&reg, token) == -1)
                                printf("%sNo es pot treure: L'usuari no existeix%s\n",VERMELL,NORMAL);
                            else printf("Usuari %s%s%s tret\n", VERD,token,NORMAL);
                        }
                    }
                }
                else printf("%sOpció no vàlida%s\n",VERMELL,NORMAL);
            }
            mostraSeparador();
            // Mostrar prompt
            printf("%s> %s",BOLDWHITE,NORMAL);
            fflush(stdout);
        }
    } while (calAcabar == 0);

    finalitzar(sckLumi,fitxerLog,&reg);

    return 0;
}


void inicialitzar(int * sckLumi, Registre * reg, int * fitxerLog) {
    char domini[100];

    if((LUMIs_IniciaServidor(sckLumi, reg, fitxerLog, domini)) == -1){
        printf("%sError iniciant el servidor: %s%s\n", VERMELL,LUMIs_ObteMissError(),NORMAL);
        exit (-1);
    }

    mostraSeparador();
    printf("%sNODE LUMI INICIALITZAT CORRECTAMENT\nDOMINI: %s%s\n",BOLDWHITE,domini,NORMAL);
}

/*
 * Copia els usuaris de la taula de registres reg al fitxer de config.
 *
 * Tanca el socket LUMI amb identificador SckLumi i el fitxer de
 * log amb identificador fitxerLog.
 */
void finalitzar(int sckLumi, int fitxerLog, const Registre * reg) {
    if(LUMIs_FinalitzaServidor(sckLumi, fitxerLog, reg) == -1) {
        printf("%sError finalitzant el servidor%s%s\n",VERMELL,LUMIs_ObteMissError(),NORMAL);
        exit(-1);
    }

    printf("S'ha finalitzat correctament\n");
}


/* Afegeix una nova entrada a la taula de registres reg amb l'usuari
 * que te per nomMI nom(cadena de caracters acabada en \0 i de mida < 35).
 * El posa offline per defecte.
 *
 * Retorna -1 si s'ha arribat al limit de la taula, -2 si l'usuari ja
 * existeix; 0 si s'ha afegit correctament.
 */
int afegirUsuari(Registre * reg, const char * nom) {
    if(reg->nElem >= MAX_REG)
        return -1;

    int pos = buscarEnRegistre(reg, nom);
    if(pos != -1)
        return -2;

    int i = reg->nElem;
    strcpy(reg->taula[i].adrecaMI,nom);
    reg->taula[i].online = 0;
    reg->nElem++;
}

/* Elimina l'entrada de la taula de registres reg on hi figura
 * l'usuari amb nomMI nom (cadena de caracters acabada en \0 i de mida <= 35,
 * incloent \0).
 *
 * Retorna -1 si l'usuari no existeix a la taula; 0 si s'ha tret
 * correctament.
 */
int treureUsuari(Registre * reg, const char * nom) {
    int i;
    int pos = buscarEnRegistre(reg, nom);
    if(pos == -1)
        return(-1);

    reg->nElem--;
    for(i = pos; i < reg->nElem; i++)
        reg->taula[i] = reg->taula[i+1];

    return 0;
}

/* Busca a la taula de registres reg, l'usuari amb nomMI nom (cadena
 * de caracters acabada en \0 i de mida < 35)
 *
 * Retorna -1 si no s'ha trobat; la posicio dins de la taula
 * de registres en cas que existeixi
 */
int buscarEnRegistre(const Registre * reg, const char * nom){
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


/* Mostra per pantalla el menu del node */
void mostrarMenu() {
    printf("%sEscull una opció:%s\n",BOLDWHITE,NORMAL);
    printf("Escriu %s/exit%s per acabar\n",BLAU,NORMAL);
    printf("Escriu %s/alta nomUsuari%s per afegir un usuari\n",BLAU,NORMAL);
    printf("Escriu %s/baixa nomUsuari%s per eliminar un usuari\n",BLAU,NORMAL);
    printf("Escriu %s/llistar%s per llistar l'estat i localització dels usuaris\n",BLAU,NORMAL);
    printf("Escriu %s/help%s per mostrar l'ajuda\n",BLAU,NORMAL);
}

/* Llista per pantalla tots els usuaris donats d'alta al node.
 * Amb el format: NOM -> SOCKET LUMI -> ESTAT
 */
void mostrarRegistre(const Registre * reg) {
    printf("%sFORMAT:%s  %sNOM  ->  SOCKET LUMI  ->  ESTAT%s\n",BOLDWHITE,NORMAL,BLAU,NORMAL);

    int i;
    for(i = 0; i < reg->nElem; i++){
        // Nom MI
        printf("%s",reg->taula[i].adrecaMI);
        printf("  ->  ");

        // Socket LUMI
        if(reg->taula[i].online == 0)
            printf("No registrat");
        else printf("%s:%d",reg->taula[i].adrecaLUMI.IP, reg->taula[i].adrecaLUMI.port);
        printf("  ->  ");

        // Estat
        if(reg->taula[i].online == 1) {
            printf("%sONLINE%s\n",VERD,NORMAL);
        }
        else {
            printf("%sOFFLINE%s\n",VERMELL,NORMAL);
        }
    }
}


/* Es mostra per pantalla una línia de '-' */
void mostraSeparador() {
    struct winsize size;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
    int i;
    for(i = 0; i < size.ws_col; i++)
        printf("-");
    printf("\n");
}

