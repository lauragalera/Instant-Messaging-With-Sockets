/**************************************************************************/
/*                                                                        */
/* P2 - MI amb sockets TCP/IP - Part II                                   */
/* Fitxer capçalera de aMI.c                                              */
/*                                                                        */
/* Autors: Laura Galera Alfaro i David Perez Sanchez                      */
/*                                                                        */
/**************************************************************************/

int MI_IniciaEscPetiRemConv(int portTCPloc);
int MI_DemanaConv(const char *IPrem, int portTCPrem, char *IPloc, int *portTCPloc, const char *NicLoc, char *NicRem);
int MI_AcceptaConv(int SckEscMI, char *IPrem, int *portTCPrem, char *IPloc, int *portTCPloc, const char *NicLoc, char *NicRem);
int MI_EnviaLinia(int SckConvMI, const char *Linia, int LongLinia);
int MI_ServeixPeticio(int SckConvMI, char *SeqBytes, int *LongSeqBytes);
int MI_AcabaConv(int SckConvMI);
int MI_AcabaEscPetiRemConv(int SckEscMI);
char* MI_ObteMissError(void);
int MI_TrobarAdreca(int Sck, char * IPloc, int * portTCPloc);
