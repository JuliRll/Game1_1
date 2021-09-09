#include "mbed.h"
#include <stdio.h>

#define TRUE    1
#define FALSE 0
#define HEARTBEATIME 200

#define NROBOTONES  4
#define MAXLED      4

#define INTERMITENCIA 6
#define TIMETOSTART 1000
#define TIMEMAX 1501
#define BASETIME 200
#define INTERVAL    40

#define ESPERAR         0
#define JUEGO           1
#define JUEGOTERMINADO  2
#define TECLAS          3


/**
 * @brief Enumeración que contiene los estados de la máquina de estados(MEF) que se implementa para 
 * el "debounce" 
 * El botón está configurado como PullUp, establece un valor lógico 1 cuando no se presiona
 * y cambia a valor lógico 0 cuando es presionado.
 */
typedef enum{
    BUTTON_DOWN,    //0
    BUTTON_UP,      //1
    BUTTON_FALLING, //2
    BUTTON_RISING   //3
}_eButtonState;

/**
 * @brief Campo de bits para controles entradas y salidas simples de verificacion
 */
typedef struct{
    uint8_t b0: 1;
    uint8_t resultado: 1;
    uint8_t jugando: 1;
}_fFlags;

_fFlags Flag;

/**
 * @brief Estructura que contiene los datos necesarios de cada boton
 */
typedef struct{
    uint8_t estado;
    int32_t timeDown;
    int32_t timeDiff;
}_sTeclas;

/**
 * @brief Cadena de estructuras, para que la cantidad de botones colocados
 * todos tengan las miemas variables (las de la estructura _sTeclas)
 */
_sTeclas ourButton[NROBOTONES];

/**
 * @brief Mascara para los valores de salida de los leds
 */
uint16_t mask[]={0x0001,0x0002,0x0004,0x0008};

uint8_t estadoJuego=ESPERAR;

/**
 * @brief Pone el estado inicial de los pulsadores en UP
 * @param indice 
 */
void startMef(uint8_t indice);

void togleLed(uint8_t index,uint8_t state);

/**
 * @brief Funcion para los estados de los pulsadores
 * Esta funcion es una maquina de estados que maneja las posiciones de los botones
 * @param indice 
 */
void actuallizaMef(uint8_t indice );

DigitalOut BEATLED(PC_13);
BusIn botones(PB_6,PB_7,PB_8,PB_9);
BusOut leds(PB_12,PB_13,PB_14,PB_15);

Timer miTimer; //!< Timer para recorrer todo el tiempo y comparar si hace falta esperar

int tiempoMs=0; //!< Variable donde voy a almacenar el tiempo del timmer una vez cumplido


int main()
{
    miTimer.start();
    uint16_t ledAuxRandom=0;
    int tiempoRandom = 0;
    int tiempoFinal = 0;
    int ledAuxRandomTime = 0;
    int ledAuxJuegoStart = 0;
    int beatTime = 0;
    int contador = 0;
    uint8_t indiceAux=0;
    uint8_t estado = 0;
    uint8_t index;

    for(uint8_t indice=0; indice<NROBOTONES;indice++){
        startMef(indice);
    }

    while(TRUE)
    {
        if((miTimer.read_ms() - beatTime) > HEARTBEATIME){
            BEATLED = !BEATLED;
            beatTime = miTimer.read_ms();
        }

        switch(estadoJuego){

            case ESPERAR:
                if ((miTimer.read_ms()-tiempoMs)>INTERVAL){
                    tiempoMs=miTimer.read_ms();
                    for(uint8_t indice=0; indice<NROBOTONES;indice++){
                        actuallizaMef(indice);
                        if(ourButton[indice].timeDiff >= TIMETOSTART){
                            srand(miTimer.read_us());
                            estadoJuego=TECLAS;   
                        }
                    }
                }
            break;

            case TECLAS:
                for( indiceAux=0; indiceAux<NROBOTONES;indiceAux++){
                    actuallizaMef(indiceAux);
                    if(ourButton[indiceAux].estado!=BUTTON_UP){
                        break;
                    }
                        
                }
                if(indiceAux==NROBOTONES){
                    leds=15;
                    ledAuxJuegoStart=miTimer.read_ms();
                    estadoJuego=JUEGO;
                    for(index = 0;index < NROBOTONES; index++){
                        startMef(index);
                    }      
                }
            break;

            case JUEGO:
                if(leds==0){
                    if((miTimer.read_ms() - ledAuxJuegoStart) > TIMETOSTART){
                        Flag.jugando = TRUE;
                    }
                    if(Flag.jugando == TRUE){
                        ledAuxRandom = rand() % (MAXLED);
                        ledAuxRandomTime = (rand() % (TIMEMAX))+BASETIME;
                        tiempoRandom=miTimer.read_ms();
                        leds=mask[ledAuxRandom];
                    }
                }else{
                    if(leds==15){
                        if((miTimer.read_ms() - ledAuxJuegoStart) > TIMETOSTART){
                            leds=0;
                            ledAuxJuegoStart = miTimer.read_ms();
                            Flag.jugando = FALSE;
                        }
                    }
                    if ((miTimer.read_ms()-tiempoMs)>INTERVAL){
                        tiempoMs=miTimer.read_ms();
                        for(uint8_t indice=0; indice< NROBOTONES;indice++){
                            actuallizaMef(indice);
                            if(ourButton[indice].estado == BUTTON_DOWN){
                                if(indice == ledAuxRandom){
                                    Flag.resultado = TRUE;
                                }else{
                                    Flag.resultado = FALSE;
                                }
                            }
                        }
                    }
                    
                    if ((miTimer.read_ms()-tiempoRandom)> ledAuxRandomTime && Flag.jugando == TRUE){
                        estadoJuego = JUEGOTERMINADO;
                    }

                    if(estadoJuego == JUEGOTERMINADO){
                        leds = FALSE;
                        tiempoFinal = miTimer.read_ms();
                    }
                }
            break;

            case JUEGOTERMINADO:
                
                if(Flag.resultado == FALSE){
                    //Perder
                    if((miTimer.read_ms() - tiempoFinal) > BASETIME){
                        estado = !estado;
                        togleLed(ledAuxRandom,estado);
                        contador++;
                        tiempoFinal = miTimer.read_ms();
                    }                    
                }else{
                    //Ganar
                    if((miTimer.read_ms() - tiempoFinal) > BASETIME){
                        estado = !estado;
                        togleLed(MAXLED,estado);
                        contador++;
                        tiempoFinal = miTimer.read_ms();
                    }
                }
                //Reinicio de botones y variables
                if(contador == INTERMITENCIA){
                    leds = FALSE;
                    contador = 0;
                    estadoJuego = ESPERAR;
                    tiempoMs = miTimer.read_ms();
                    for(uint8_t index = 0;index < NROBOTONES ;index++){
                        ourButton[index].timeDiff = 0;
                        ourButton[index].timeDown = 0;
                        startMef(index);
                    }
                    Flag.jugando = FALSE;
                    Flag.resultado = FALSE;
                }

                break;
            default:
                estadoJuego=ESPERAR;
        }
    }
    return 0;
}



void startMef(uint8_t indice){
   ourButton[indice].estado=BUTTON_UP;
}


void actuallizaMef(uint8_t indice){

    switch (ourButton[indice].estado)
    {
        case BUTTON_DOWN:
            if(botones.read() & mask[indice] )
                ourButton[indice].estado=BUTTON_RISING;
        break;

        case BUTTON_UP:
            if(!(botones.read() & mask[indice]))
                ourButton[indice].estado=BUTTON_FALLING;
        break;

        case BUTTON_FALLING:
            if(!(botones.read() & mask[indice]))
            {
                ourButton[indice].timeDown=miTimer.read_ms();
                ourButton[indice].estado=BUTTON_DOWN;
            }
            else
                ourButton[indice].estado=BUTTON_UP;    
        break;

        case BUTTON_RISING:
            if(botones.read() & mask[indice]){
                ourButton[indice].estado=BUTTON_UP;
                ourButton[indice].timeDiff=miTimer.read_ms()-ourButton[indice].timeDown;
            }
            else
                ourButton[indice].estado=BUTTON_DOWN;
        break;
        
        default:
            startMef(indice);
        break;
    }
}
void togleLed(uint8_t index,uint8_t state){
    switch (state){
        case TRUE:
            leds = mask[index];
            break;
        case FALSE:
            leds = FALSE;
        default:
            break;
    }
}