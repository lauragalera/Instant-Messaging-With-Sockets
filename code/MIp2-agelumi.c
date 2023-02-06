/**************************************************************************/
/*                                                                        */
/* P2 - MI amb sockets TCP/IP - Part II                                   */
/* Fitxer MIp2-agelumi.c que implementa la interfície aplicació-usuari    */
/* d'un agent d'usuari de LUMI sol, no integrat a l'aplicació de MI,      */
/* sobre la capa d'aplicació de (la part client de ) LUMI (fent crides    */
/* a la interfície de la part client de LUMI).                            */
/* Autors: Laura Galera Alfaro i David Pérez Sánchez                      */
/*                                                                        */
/**************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include "MIp2-aLUMIc.h"
#include "MIp2-aA.h"

/* Definició de constants, p.e.,                                          */

/* #define XYZ       1500                                                 */

/* Declaració de funcions INTERNES que es fan servir en aquest fitxer     */
/* (les  definicions d'aquestes funcions es troben més avall) per així    */
/* fer-les conegudes des d'aquí fins al final d'aquest fitxer, p.e.,      */

/* int FuncioInterna(arg1, arg2...);                                      */



int main(int argc,char *argv[])
{
    int socket;
    char IPnode[16];
    char IPdesti[16];
    int portdesti;
    char mi1[200];
    char mi2[200];
    int fitxerLog;


    if((LUMIc_IniciaClient(&socket,&fitxerLog)) == -1){
        perror(LUMIc_ObteMissError());
        exit(-1);
    }

    printf("Entra la teva adreça MI (mida MAX 36 caràcters)\n");
    scanf("%36s",mi1);

    int aux = LUMIc_DemanaReg(socket,mi1,fitxerLog);
    if(aux == -1){
        perror(LUMIc_ObteMissError());
        exit(-1);
    }

    if(aux == 0){
        printf("Enregistrament acceptat: s'ha registrat correctament al node\n");
    }else{
        printf("Enregistrament rebutjat: usuari no donat d'alta\n");
    }

    printf("Prem intro si vols demanar localització\n");

    int llista[2] = {0,socket};
    int activat = A_HaArribatAlgunaCosa(llista,2);

    if(activat == 0) {
        printf("Entra la adreça MI del destí (mida MAX 36 caràcters)\n");
        scanf("%36s",mi2);

        aux = LUMIc_DemanaLoc(socket,mi1,mi2,IPdesti,&portdesti,fitxerLog);
        switch (aux) {
            case 0:
                printf("S'ha pogut localitzar al company de conversa\n");
                break;
            case 1:
                printf("Company ocupat\n");
                break;
            case 2:
                printf("Company o domini inexistent\n");
                break;
            case 3:
                printf("Company offline\n");
                break;
            default:
                perror(LUMIc_ObteMissError());
                exit(-1);
        }
    }
    else if(activat == socket) {
        aux = LUMIc_RespondreLoc(socket,0,"192.168.1.53",1234,fitxerLog);
        if(aux == -1) {
            perror(LUMIc_ObteMissError());
            exit(-1);
        }
    }
    else {
        perror(A_ObteMissError());
        exit(-1);
    }

    aux = LUMIc_DemanaDesReg(socket,mi1,fitxerLog);
    if(aux == -1){
        perror(LUMIc_ObteMissError());
        exit(-1);
    }

    if(aux == 0){
        printf("Desregistrament acceptat: s'ha desregistrat correctament al node\n");
    }else{
        printf("Desregistrament rebutjat: usuari no existeix\n");
    }

    if((LUMIc_FinalitzaClient(socket,fitxerLog)) == -1){
        perror(LUMIc_ObteMissError());
        exit(-1);
    }
}
