/* Bibliotecas */
#include "mbed.h"


/* Pinagem */
#define sensor_apps1  PA_5                          // Sinal de entrada do sensor do apps 1
#define sensor_apps2  PA_6                          // Sinal de entrada do sensor do apps 2
#define sensor_bse    PB_0                          // Sinal de entrada do sensor do bse
#define pino_inversor PA_4                          // Sinal de saída para o inversor //Isso


/* Estrutura de dados */
typedef enum {FALHA, TORQUE} state;


/* Classes API */
AnalogIn apps1_aq(sensor_apps1);
AnalogIn apps2_aq(sensor_apps2);
AnalogIn bse_aq(sensor_bse);
AnalogOut inversor_envio(pino_inversor);


/* Variaveis */
// Variaveis para a medicao de erro
bool flag_falha;
float apps1;
float apps2;
float bse;
float inversor;

//Declaraveis
float max_apps1;
float min_apps1;
float max_apps2;
float min_apps2; 

//Maquina de estados
state estado;

/* Funcoes */
void convertevalores();
int funcao_falha();


int main()
{
    while (true) {
        switch (estado) {

            case FALHA:
                inversor_envio.write(0);
                break;

            case TORQUE:
                inversor_envio.write(apps1);
                printf("Enviando %f de tensao", apps1_aq.read_voltage());
                break;
        }
    }
}

 
void convertevalores () {
    /*
    apps1 = apps1_aq.read_voltage();
    apps2 = apps2_aq.read_voltage();
    bse = bse_aq.read_voltage();
    */
    //Valores convertidos em porcentagem
    apps1 = (apps1_aq.read_voltage() - min_apps1) / (max_apps1 - min_apps1);
    apps2 = (apps2_aq.read_voltage() - min_apps2) / (max_apps2 - min_apps2);
}


int funcao_falha() {
    convertevalores();
    if ( (apps1 > (0.1*apps2)) || (apps1 < (0.1*apps2)) ) {
        printf("Erro de 10 porcento entre os sensores, %f e %f \n", apps1_aq.read(), apps2_aq.read_voltage());
        return 0;
    }
    else {
        if ( (apps1 > max_apps1) ) {
            printf("Apps1 acima da escala maxima, %f e %f \n", apps1_aq.read(), apps2_aq.read_voltage());
            return 0;
        }
        else {
            if( (apps2 > max_apps2) ) {
                printf("Apps2 acima da escala maxima, %f e %f \n", apps1_aq.read(), apps2_aq.read_voltage());
                return 0;
            }
            else {
                if  (apps1 < min_apps1 ) {
                    printf ("Apps1 abixo da escala minima, %f e %f \n", apps1_aq.read(), apps2_aq.read_voltage());
                    return 0;
                }
                else {
                    if (apps2 < min_apps2) {
                        printf("Apps2 abaixo da escala minima, %f e %f \n", apps1_aq.read(), apps2_aq.read_voltage());
                        return 0;
                    }
                    else {
                                return 1;
                    }
                }
            }
        }
    }
}