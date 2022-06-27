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
float max_apps1 = 4.5;
float min_apps1 = 0.5;
float max_apps2 = 4.5;
float min_apps2 = 0.5; 

//Maquina de estados
state estado;


/* Funcoes */
void convertevalores();
void funcao_falha();


int main() {

    while (true) {

        funcao_falha();

        switch (estado) {

            case FALHA:
                inversor_envio.write(0);
                printf("Dentro da FALHA, mandando 0V \n");
                break;

            case TORQUE:
                inversor_envio.write(apps1);
                printf("Enviando %f de tensao \n", inversor_envio.read()*3.3);
                break;
        }
    }
}

 
void convertevalores () {
    apps1_aq.set_reference_voltage(3.3);
    apps2_aq.set_reference_voltage(3.3);
    apps1 = (apps1_aq.read_voltage() - min_apps1) / (max_apps1 - min_apps1);
    apps2 = (apps2_aq.read_voltage() - min_apps2) / (max_apps2 - min_apps2);
}


void funcao_falha() {

    convertevalores();

    //printf("Apps1 = %f e Apps2 = %f \n", apps1_aq.read_voltage(), apps2_aq.read_voltage());
    if ( (apps1 > (1.1*apps2)) || (apps1 < (0.9*apps2)) ) {
        printf("Erro de 10 porcento entre os sensores, %f e %f \n", apps1_aq.read_voltage(), apps2_aq.read_voltage());
        estado = FALHA; 
    }
    else {
        if ( (apps1_aq.read_voltage() > max_apps1) ) {
            printf("Apps1 acima da escala maxima, %f e %f \n", apps1_aq.read_voltage(), apps2_aq.read_voltage());
            estado = FALHA;
        }
        else {
            if( (apps2_aq.read_voltage() > max_apps2) ) {
                printf("Apps2 acima da escala maxima, %f e %f \n", apps1_aq.read_voltage(), apps2_aq.read_voltage());
                estado = FALHA;
            }
            else {
                if  (apps1_aq.read_voltage() < min_apps1 ) {
                    printf ("Apps1 abixo da escala minima, %f e %f \n", apps1_aq.read_voltage(), apps2_aq.read_voltage());
                    estado = FALHA;
                }
                else {
                    if (apps2_aq.read_voltage() < min_apps2) {
                        printf("Apps2 abaixo da escala minima, %f e %f \n", apps1_aq.read_voltage(), apps2_aq.read_voltage());
                        estado = FALHA;
                    }
                    else {
                        estado = TORQUE;
                    }
                }
            }
        }
    }
}