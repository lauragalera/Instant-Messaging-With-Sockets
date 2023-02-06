/**************************************************************************/
/*                                                                        */
/* Aplicacio P2P                                                          */
/* Autors: David Pérez Sánchez, Laura Galera Alfaro                       */
/*                                                                        */
/**************************************************************************/

#include "MIp2-aMI.h"
#include "MIp2-aLUMIc.h"
#include "MIp2-aA.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

// Colors per la interficie
#define BOLDWHITE   "\033[1m\033[37m"
#define BLAU  "\x1B[36m"
#define VERD  "\x1B[32m"
#define VERMELL  "\x1B[31m"
#define NORMAL  "\x1B[0m"

// Accions
void inicialitzar(int * sesc, int * sckLumi, char * IPlocalEsc, int * portLocalEsc, int * fitxerLog, const char * nick);
void finalitzar(int sesc, int sckLumi, const char * adrecaMIlocal, int fitxerLog);

// Funcions
void obtenirIPportEsc(int sesc, char * IPesc, int * portesc);
void mostrarInfo(const char * nickLocal, const char * nickRemot, const char * ipLocal, const int portLocal,
                 const char * ipRemot, const int portRemot);
void mostraSeparador();

int main(int argc,char *argv[])
{
    /*****************************************************/
    /**            Declaracio de variables              **/
    /*****************************************************/
    int sesc, scon;
    int fd;
    char buffer[200];
    int calContinuar;
    int destiTrobat;
    int long_buffer;
    int llistaFD[3];

    char IPlocalConv[16];
    int portLocalConv;
    char IPlocalEsc[16];
    int portLocalEsc;
    int portRemot;
    char ipRemot[16];

    char nickLocal[100];
    char nickRemot[100];

    int sckLumi;
    char adrecaMIlocal[100];
    char adrecaMIremot[100];
    int fitxerLog;

    /*****************************************************/
    /**                 Codi principal                  **/
    /*****************************************************/

    // Mostrar benvinguda
    mostraSeparador();
    printf("%sClient d'aplicació MI%s\n",BOLDWHITE,NORMAL);

    // Login
    mostraSeparador();
    printf("%sLOGIN%s\n",BOLDWHITE,NORMAL);
    printf("Introdueix el teu nick (màxim 20 caràcters):\n%s> %s", BOLDWHITE, NORMAL);
    scanf("%20s",nickLocal);

    inicialitzar(&sesc, &sckLumi, IPlocalEsc, &portLocalEsc, &fitxerLog, nickLocal);

    int aux;
    do {
        // Demanar adreca MI local
        printf("Entra @MI (mida MAX 36 caràcters) %snomMI@domini%s:\n%s> %s", BLAU, NORMAL, BOLDWHITE, NORMAL);
        scanf("%36s", adrecaMIlocal);

        // Demanar registre al node LUMI
        aux = LUMIc_DemanaReg(sckLumi, adrecaMIlocal, fitxerLog);
        switch (aux) {
            case 0:
                printf("%sS'ha registrat correctament al node%s\n",VERD,NORMAL);
                break;
            case -1:
                printf("%sNode no respon: %s%s\n", VERMELL,LUMIc_ObteMissError(),NORMAL);
                break;
            case -2:
                printf("%sFormat de l'adreça incorrecte: %s%s\n",VERMELL,LUMIc_ObteMissError(),NORMAL);
                break;
            case -3:
                printf("%sNo s'ha pogut trobar el domini: %s%s\n",VERMELL,LUMIc_ObteMissError(),NORMAL);
                break;
            case 1:
                printf("%sUsuari no existent%s\n",VERMELL, NORMAL);
                break;
            default:
                break;
        }
    } while (aux != 0);


    do {
        mostraSeparador();
        // Inicialitzar variables de control
        destiTrobat = calContinuar =  0;
        printf("Prem %sENTER%s per crear una connexió o %sESPERA%s a una petició de connexió...\n",BLAU,NORMAL,BLAU,NORMAL);
        printf("Entra %sEXIT%s per tancar el programa\n",BLAU,NORMAL);

        llistaFD[0] = 0;
        llistaFD[1] = sckLumi;
        llistaFD[2] = sesc;
        int activat;

        do {
            if((activat = A_HaArribatAlgunaCosa(llistaFD, 3)) == -1) {
                printf("%sError mirant si ha arribat alguna cosa: %s%s\n", VERMELL, A_ObteMissError(), NORMAL);
                exit(-1);
            }
            if (activat == sckLumi) {
                // Respondre amb IP i port de conversa
                if(LUMIc_RespondreLoc(sckLumi, 0, IPlocalEsc, &portLocalEsc, fitxerLog) == -1) {
                    printf("%sError responent localitzacio: %s%s\n",VERMELL,LUMIc_ObteMissError(),NORMAL);
                    exit(-1);
                }
            }
        } while (activat == sckLumi);

        if (activat == 0) {
            char opcio[10];
            int bytes_llegits;

            // Llegir de teclat
            if((bytes_llegits = read(0,opcio,sizeof(opcio))) == -1){
                printf("%sError en el read%s",VERMELL,NORMAL);
                return (-1);
            }
            opcio[bytes_llegits-1] = '\0';

            calContinuar = (strcmp(opcio,"EXIT") != 0);

            if(calContinuar == 1) {
                printf("Entra @MI desti (mida MAX 36 caràcters) %snomMI@domini%s:\n%s> %s", BLAU, NORMAL, BOLDWHITE, NORMAL);
                scanf("%36s", adrecaMIremot);

                // Demanar IP i port de conversa
                aux = LUMIc_DemanaLoc(sckLumi, adrecaMIlocal, adrecaMIremot, ipRemot, &portRemot, fitxerLog);
                switch (aux) {
                    case 0:
                        printf("%sS'ha pogut localitzar al company de conversa%s\n",VERD,NORMAL);
                        // Iniciar conversa
                        if ((scon = MI_DemanaConv(ipRemot, portRemot, IPlocalConv, &portLocalConv, nickLocal, nickRemot)) == -1) {
                            printf("%sError demanant conversa: %s%s\n",VERMELL,MI_ObteMissError(),NORMAL);
                            exit(-1);
                        }
                        destiTrobat = 1;
                        break;
                    case 1:
                        printf("%sCompany ocupat%s\n",VERMELL,NORMAL);
                        break;
                    case 2:
                        printf("%sCompany o domini inexistent%s\n",VERMELL,NORMAL);
                        break;
                    case 3:
                        printf("%sCompany offline%s\n",VERMELL,NORMAL);
                        break;
                    default:
                        printf("%sError demanant localitzacio: %s%s\n",VERMELL,LUMIc_ObteMissError(),NORMAL);
                }
            }
        }
        else {
            // Iniciar conversa
            if ((scon = MI_AcceptaConv(sesc, ipRemot, &portRemot, IPlocalConv, &portLocalConv, nickLocal, nickRemot)) == -1) {
                printf("%sError acceptant conversa: %s%s\n", VERMELL, MI_ObteMissError(), NORMAL);
                exit(-1);
            }
            calContinuar = destiTrobat = 1;
        }

        // Iniciar conversa
        if(calContinuar == 1 && destiTrobat == 1) {
            // Mostrar informacio de la connexion i els nicknames
            mostrarInfo(nickLocal, nickRemot, IPlocalConv, portLocalConv, ipRemot, portRemot);

            // Emplenar llista de file descriptors amb el teclat i el socket
            llistaFD[0] = 0;
            llistaFD[1] = sckLumi;
            llistaFD[2] = scon;

            // Conversar
            int tipus = 1;
            do {
                if ((fd = A_HaArribatAlgunaCosa(llistaFD, sizeof(llistaFD) / sizeof(int))) == -1) {
                    printf("%sError mirant si ha arribat alguna cosa: %s%s\n", VERMELL, A_ObteMissError(), NORMAL);
                    exit(-1);
                }
                //Si s'ha escrit alguna cosa per teclat...
                if (fd == 0) {
                    if ((long_buffer = read(0, buffer, sizeof(buffer))) == -1) {
                        printf("%sError en el read%s\n", VERMELL, NORMAL);
                        exit(-1);
                    }
                    buffer[long_buffer - 1] = '\0';
                    int esExit_o_SaltLinia = strcmp(buffer, "/exit") == 0 || strcmp(buffer, "\0") == 0;
                    //Si no es tracta d'exit o de salt de linia, s'envia
                    if (esExit_o_SaltLinia == 0) {
                        // S'envia la linia
                        if (MI_EnviaLinia(scon, buffer, strlen(buffer)) == -1) {
                            printf("%sError enviant linia: %s%s\n", VERMELL, MI_ObteMissError(), NORMAL);
                            exit(-1);
                        }
                    }
                }
                //Si s'ha rebut alguna cosa pel socket i no s'ha demanat tancar la connexio previament...
                if (fd == scon && strcmp(buffer, "/exit") != 0) {
                    //Es llegeix allo rebut
                    if ((tipus = MI_ServeixPeticio(scon, buffer, &long_buffer)) == -1) {
                        printf("%sError servint peticio: %s%s\n",VERMELL,MI_ObteMissError(),NORMAL);
                        exit(-1);
                    }
                    // Si es un missatge, es mostra per pantalla
                    if (tipus == 1) {
                        printf("%s diu: %s\n", nickRemot, buffer);
                    }
                }
                //Si algu mes vol establir connexio
                if (fd == sckLumi) {
                    if (LUMIc_RespondreLoc(sckLumi, 1, NULL, NULL, fitxerLog) == -1) {
                        printf("%sError responent localitzacio: %s%s\n",VERMELL,LUMIc_ObteMissError(),NORMAL);
                        exit(-1);
                    }
                }
            } while (strcmp(buffer, "/exit") != 0 &&
                     tipus > 0); //S'acaba la conversa perquè s'ha llegit /exit o han tancat el socket

            // Tancar socket conversa
            if (MI_AcabaConv(scon) == -1) {
                printf("%sError acabant conversa: %s%s\n",VERMELL,MI_ObteMissError(),NORMAL);
                exit(-1);
            }
        }
    } while(calContinuar == 1);

    finalitzar(sesc, sckLumi, adrecaMIlocal, fitxerLog);

    return (0);
}

/*
 * Inicia el socket d'escolta amb identificador sesc, omplint IPlocalEsc (cadena de caracters acabada en \0
 * de mida maxima 16, incloent el \0) i portLocalEsc amb la IP i el port del socket d'escolta.
 *
 * Crea un fitxer de log l'identificador del qual es fitxerLog i de nom p2p-nick.log. El nick
 * es una cadena de caracters acabada en \0 i de mida maxima 21, incloent \0.
 */
void inicialitzar(int * sesc, int * sckLumi, char * IPlocalEsc, int * portLocalEsc, int * fitxerLog, const char * nick) {
    // Crear el socket sesc d'escolta i preparar-lo per escoltar peticions
    if((*sesc = MI_IniciaEscPetiRemConv(0)) == -1) {
        printf("%sError iniciant escolta de peticions de conversa: %s%s\n", VERMELL, MI_ObteMissError(), NORMAL);
        exit(-1);
    }

    obtenirIPportEsc(*sesc, IPlocalEsc, portLocalEsc);

    // Iniciar agent LUMI
    char buff[200];
    sprintf(buff,"p2p-%s.log",nick);
    if((LUMIc_IniciaClient(sckLumi,fitxerLog,buff)) == -1){
        printf("%sError iniciant el client LUMI: %s%s\n", VERMELL, LUMIc_ObteMissError(), NORMAL);
        exit(-1);
    }
}

/*
 * Desregistra a l'usuari local amb adreca MI adrecaMIlocal (cadena de caracters acabada en \0 i de mida
 * maxima 36), prenent nota en fitxer de log amb identificador fitxerLog.
 *
 * Tanca el socket d'escolta amb identificador sesc, el socket LUMI amb
 * identificador sckLumi i el fitxer de log.
 */
void finalitzar(int sesc, int sckLumi, const char * adrecaMIlocal, int fitxerLog) {
    // Tancar socket escolta
    if(MI_AcabaEscPetiRemConv(sesc) == -1){
        printf("%sError acabant escolta de peticions de conversa: %s%s\n", VERMELL,MI_ObteMissError(),NORMAL);
        exit(-1);
    }

    // Es desregistra l'usuari del node
    int aux = LUMIc_DemanaDesreg(sckLumi,adrecaMIlocal,fitxerLog);
    if(aux == -1){
        printf("%sNode no respon: %s%s\n", VERMELL, LUMIc_ObteMissError(), NORMAL);
        exit(-1);
    }
    else if(aux == 1){
        printf("%sUsuari no existent%s\n", VERMELL, NORMAL);
        exit(-1);
    }

    printf("\n%sT'has desregistrat del teu domini%s\n", VERD, NORMAL);

    // Tancar socket LUMI
    if(LUMIc_FinalitzaClient(sckLumi,fitxerLog) == -1){
        printf("%sError finalitzant el client LUMI: %s%s\n", VERMELL, LUMIc_ObteMissError(),NORMAL);
        exit(-1);
    }

    printf("Fi execució\n");
}


/*
 * Omple IPesc (cadena de caracters acabada en \0 i de mida maxima 16, incloent el \0)
 * i portesc amb la IP i el port TCP on s'escolten peticions de connexio.
 *
 * sesc es l'identificador del socket d'escolta.
 */
void obtenirIPportEsc(int sesc, char * IPesc, int * portesc) {
    FILE *f;

    // Trobar port d'escolta
    if(MI_TrobarAdreca(sesc, IPesc, portesc) == -1){
        printf("%sError trobant adreca d'escolta: %s%s\n",VERMELL,MI_ObteMissError(),NORMAL);
        exit(-1);
    }

    // Crida al sistema
    f = popen("/bin/hostname -I", "r");
    if (f == NULL) {
        printf("%sError en el popen%s\n", VERMELL, NORMAL);
        exit(-1);
    }

    // Obtenir linia amb la IP
    fscanf(f,"%16s",IPesc);

    // Tancar f
    pclose(f);
}

/* Mostra per pantalla una línia de '-' */
void mostraSeparador() {
    struct winsize size;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
    int i;
    for(i = 0; i < size.ws_col; i++)
        printf("-");
    printf("\n");
}

/* Mostra per pantalla el nickname del local i del remot, la IP i el port del local
 * i la IP i el port del remot, tot entre separadors.
 */
void mostrarInfo(const char * nickLocal, const char * nickRemot, const char * ipLocal, const int portLocal,
                 const char * ipRemot, const int portRemot) {
    mostraSeparador();
    printf("Nickname Local -> %s\nNickname Remot -> %s\n\n",nickLocal,nickRemot);
    printf("ADREÇA LOCAL -> %s:%d\n",ipLocal,portLocal);
    printf("ADREÇA REMOTA -> %s:%d\n",ipRemot,portRemot);
    mostraSeparador();
}