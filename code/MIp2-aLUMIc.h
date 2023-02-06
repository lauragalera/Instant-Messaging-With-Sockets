/**************************************************************************/
/*                                                                        */
/* P2 - MI amb sockets TCP/IP - Part II                                   */
/* Fitxer capçalera de aLUMIc.c                                           */
/*                                                                        */
/* Autors: Laura Galera Alfaro i David Pérez Sánchez                      */
/*                                                                        */
/**************************************************************************/

int LUMIc_IniciaClient(int * Sck, int * fitxerLog, const char * nomFitxer);
int LUMIc_FinalitzaClient(int Sck, int fitxerLog);
int LUMIc_DemanaReg(int Sck, const char * adrecaMI, int fitxerLog);
int LUMIc_DemanaDesreg(int Sck, const char * adrecaMI, int fitxerLog);
int LUMIc_DemanaLoc(int Sck, const char * adrecaMI1, const char * adrecaMI2, char * IPremot, int * portremot, int fitxerLog);
int LUMIc_RespondreLoc(int Sck, int codi, const char * IPMI, const int * portMI, int fitxerLog);
char* LUMIc_ObteMissError(void);