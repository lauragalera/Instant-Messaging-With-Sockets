/**************************************************************************/
/*                                                                        */
/* P2 - MI amb sockets TCP/IP - Part II                                   */
/* Fitxer capçalera de aLUMIs.c                                           */
/*                                                                        */
/* Autors: Laura Galera Alfaro i David Perez Sanchez                      */
/*                                                                        */
/**************************************************************************/

#define MAX_REG 100

#ifndef REGISTRE
#define REGISTRE
typedef struct {
    char IP[16];
    int port;
} Adreca;

typedef struct {
    char adrecaMI[100];
    Adreca adrecaLUMI;
    int online;
    int intents;
} Entrada;

typedef struct {
    Entrada taula[MAX_REG];
    int nElem;
} Registre;
#endif

/* Funcions externes*/
int LUMIs_IniciaServidor(int * Sck, Registre * reg, int * fitxerLog, char * domini);
int LUMIs_FinalitzaServidor(const int Sck, const int fitxerLog, const Registre * reg);
int LUMIs_ServeixPeticio(const int Sck, Registre * reg, const int fitxerLog);
char* LUMIs_ObteMissError(void);
