#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h> 
#include <time.h> 

#define MAX_SENSORES 1000 
#define TAMANHO_LINHA 256 

// estrutura para armazenar os dados de cada sensor
typedef struct {
    char id[20];
    double soma_temperatura;
    double soma_quadrados_temp;
    int contagem_temp;
} SensorTemp;

int main(int argc, char *argv[]) {
    FILE *arquivo = fopen(argv[1], "r");
    if (arquivo == NULL) {
        printf("Erro ao abrir o arquivo %s.\n", argv[1]);
        return 1;
    }

    // tempo de inicio 
    clock_t tempo_inicio = clock();

    // array para os sensores de temperatura 
    SensorTemp sensores[MAX_SENSORES];
    int total_sensores_temp = 0;

    // variaveis para os totais
    int total_alertas = 0;
    double consumo_total_energia = 0.0;

    char linha[TAMANHO_LINHA];

    while (fgets(linha, sizeof(linha), arquivo) != NULL) {
        char id_sensor[50], data[20], hora[20], tipo[50], status[50], ignorar[20];
        double valor;

        if (sscanf(linha, "%s %s %s %s %lf %s %s", id_sensor, data, hora, tipo, &valor, ignorar, status) == 7) {
            
            // verifica se possui um status de alerta ou critico 
            // estou contando que alerta siginifica nao estar no estado padrao que seria o OK entao critico tambem contaria
            // caso nao conte, so sera necessario tirar o trecho “|| strcmp(status, "CRITICO")“
            if (strcmp(status, "ALERTA") == 0 || strcmp(status, "CRITICO") == 0) {
                total_alertas++;
            }

            //soma o consumo de energia
            if (strcmp(tipo, "energia") == 0) {
                consumo_total_energia += valor;
            }

            // atualiza as estatísticas do sensor se for temperatura
            if (strcmp(tipo, "temperatura") == 0) {
                int encontrado = 0;
                
                // ve se o sensor já existe no array
                for (int i = 0; i < total_sensores_temp; i++) {
                    if (strcmp(sensores[i].id, id_sensor) == 0) {
                        sensores[i].soma_temperatura += valor;
                        sensores[i].soma_quadrados_temp += (valor * valor);
                        sensores[i].contagem_temp++;
                        encontrado = 1;
                        break;
                    }
                }

                // cadastra um sensor de temperatura que aparece pela primeira vez
                if (encontrado == 0 && total_sensores_temp < MAX_SENSORES) {
                    strcpy(sensores[total_sensores_temp].id, id_sensor);
                    sensores[total_sensores_temp].soma_temperatura = valor;
                    sensores[total_sensores_temp].soma_quadrados_temp = (valor * valor);
                    sensores[total_sensores_temp].contagem_temp = 1;
                    total_sensores_temp++;
                }
            }
        }
    }

    fclose(arquivo);

    // tempo 
    clock_t tempo_fim = clock();
    double tempo_execucao = (double)(tempo_fim - tempo_inicio) / CLOCKS_PER_SEC;

    // sensor mais instavel 
    char id_mais_instavel[20] = "";
    //comeca em -1 para fazer a comparacao com o primeiro elemento, ja que ele ira comecar com um numero maior que -1 ai ele ira comparar e virar o maior
    double maior_desvio_padrao = -1.0;
    
    printf("Media de temperatura dos 10 primeiros :\n");

    for (int i = 0; i < total_sensores_temp; i++) {      
        // Calculo media variancia e desvio padrão
        double media = sensores[i].soma_temperatura / sensores[i].contagem_temp;
        double media_dos_quadrados = sensores[i].soma_quadrados_temp / sensores[i].contagem_temp;
        double variancia = media_dos_quadrados - (media * media);
        double desvio_padrao = sqrt(variancia);

        if (desvio_padrao > maior_desvio_padrao) {
            maior_desvio_padrao = desvio_padrao;
            strcpy(id_mais_instavel, sensores[i].id);
        }

        if (i < 10) {
            printf("%s:%.2f \n", sensores[i].id, media);
        }
    }

    printf("\nsensor mais instavel e seu desvio padrao:%s  %.2f\n", id_mais_instavel, maior_desvio_padrao);
    printf("total de alertas: %d\n", total_alertas);
    printf("consumo total de energia: %.2f\n", consumo_total_energia);
    printf("Tempo de execucao: %.4f segundos\n", tempo_execucao);

    return 0;
}