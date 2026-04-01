#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h> 

#define MAX_SENSORES 1000
#define TAMANHO_LINHA 256

// estrutura para armazenar os dados de cada sensor
typedef struct {
    char id[20];
    double soma_temperatura;
    double soma_quadrados_temp;
    int contagem_temp;
} SensorTemp;

typedef struct {
    char nome_arquivo[256];
    long inicio;
    long fim;
    
    //cada thread tem seus resultados
    SensorTemp sensores_locais[MAX_SENSORES];
    int total_sensores_locais;
    int alertas_locais;
    double energia_local;
} ArgumentosThread;

void *processar_arquivo(void *arg) {
    ArgumentosThread *argumentos = (ArgumentosThread *)arg;
    
    argumentos->total_sensores_locais = 0;
    argumentos->alertas_locais = 0;
    argumentos->energia_local = 0.0;
    
    FILE *arquivo = fopen(argumentos->nome_arquivo, "r");
    if (arquivo == NULL) {
        return NULL;
    }

    fseek(arquivo, argumentos->inicio, SEEK_SET);

    if (argumentos->inicio > 0) {
        char caractere;
        while (fread(&caractere, 1, 1, arquivo) == 1) {
            if (caractere == '\n') {
                break;
            }
        }
    }

    char linha[TAMANHO_LINHA];

    while (ftell(arquivo) < argumentos->fim && fgets(linha, sizeof(linha), arquivo) != NULL) {
        char id_sensor[50], data[20], hora[20], tipo[50], status[50], ignorar[20];
        double valor;


        if (sscanf(linha, "%s %s %s %s %lf %s %s", id_sensor, data, hora, tipo, &valor, ignorar, status) == 7) {
            
            if (strcmp(status, "ALERTA") == 0 || strcmp(status, "CRITICO") == 0) {
                argumentos->alertas_locais++;
            }

            if (strcmp(tipo, "energia") == 0) {
                argumentos->energia_local += valor;
            }

            if (strcmp(tipo, "temperatura") == 0) {
                int encontrado = 0;
                
                for (int i = 0; i < argumentos->total_sensores_locais; i++) {
                    if (strcmp(argumentos->sensores_locais[i].id, id_sensor) == 0) {
                        argumentos->sensores_locais[i].soma_temperatura += valor;
                        argumentos->sensores_locais[i].soma_quadrados_temp += (valor * valor);
                        argumentos->sensores_locais[i].contagem_temp++;
                        encontrado = 1;
                        break;
                    }
                }

                if (encontrado == 0 && argumentos->total_sensores_locais < MAX_SENSORES) {
                    strcpy(argumentos->sensores_locais[argumentos->total_sensores_locais].id, id_sensor);
                    argumentos->sensores_locais[argumentos->total_sensores_locais].soma_temperatura = valor;
                    argumentos->sensores_locais[argumentos->total_sensores_locais].soma_quadrados_temp = (valor * valor);
                    argumentos->sensores_locais[argumentos->total_sensores_locais].contagem_temp = 1;
                    argumentos->total_sensores_locais++;
                }
            }
        }
    }

    fclose(arquivo);
    return NULL;
}

int main(int argc, char *argv[]) {
    int num_threads = atoi(argv[2]);
    if (num_threads <= 0) {
        printf("o numero de threads deve ser maior que zero.\n");
        return 1;
    }

    clock_t tempo_inicio = clock();

    FILE *arquivo = fopen(argv[1], "r");
    if (arquivo == NULL) {
        printf("erro ao abrir o arquivo %s.\n", argv[1]);
        return 1;
    }

     // vai ate o final do arquivo pra descobrir o tamanho do arquivo
    fseek(arquivo, 0, SEEK_END);
    long tamanho_arquivo = ftell(arquivo);
    fclose(arquivo);

    // divide o tamanho total pelo numero de threads
    long tamanho_bloco = tamanho_arquivo / num_threads;

    pthread_t threads[num_threads];
    ArgumentosThread argumentos[num_threads];

    for (int i = 0; i < num_threads; i++) {
        strcpy(argumentos[i].nome_arquivo, argv[1]);
        argumentos[i].inicio = i * tamanho_bloco;
        
        // caso tenham numeros de threads impar a ultima thread vai ate o final para mapear tudo
        if (i == num_threads - 1) {
            argumentos[i].fim = tamanho_arquivo;
        } else {
            argumentos[i].fim = (i + 1) * tamanho_bloco;
        }
       // cria a thread e executa a função 'processar_arquivo'
        pthread_create(&threads[i], NULL, processar_arquivo, &argumentos[i]);
    }

    //garante que todas as threads terminem o trabalho
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    //soma os dados sem comcorrencia
    SensorTemp sensores_globais[MAX_SENSORES];
    int total_sensores_temp_global = 0;
    int total_alertas_global = 0;
    double consumo_total_energia_global = 0.0;

    for (int t = 0; t < num_threads; t++) {
        total_alertas_global += argumentos[t].alertas_locais;
        consumo_total_energia_global += argumentos[t].energia_local;

        for (int i = 0; i < argumentos[t].total_sensores_locais; i++) {
            int encontrado_global = 0;
            
            for (int j = 0; j < total_sensores_temp_global; j++) {
                if (strcmp(sensores_globais[j].id, argumentos[t].sensores_locais[i].id) == 0) {
                    sensores_globais[j].soma_temperatura += argumentos[t].sensores_locais[i].soma_temperatura;
                    sensores_globais[j].soma_quadrados_temp += argumentos[t].sensores_locais[i].soma_quadrados_temp;
                    sensores_globais[j].contagem_temp += argumentos[t].sensores_locais[i].contagem_temp;
                    encontrado_global = 1;
                    break;
                }
            }
            // cadastra um sensor de temperatura que aparece pela primeira vez
            if (encontrado_global == 0 && total_sensores_temp_global < MAX_SENSORES) {
                sensores_globais[total_sensores_temp_global] = argumentos[t].sensores_locais[i];
                total_sensores_temp_global++;
            }
        }
    }
    
    // tempo 
    clock_t tempo_fim = clock();
    double tempo_execucao = (double)(tempo_fim - tempo_inicio) / CLOCKS_PER_SEC;

    // sensor mais instavel 
    char id_mais_instavel[20] = "";
    //comeca em -1 para fazer a comparacao com o primeiro elemento, ja que ele ira comecar com um numero maior que -1 ai ele ira comparar e virar o maior
    double maior_desvio_padrao = -1.0;
    
 
    printf("media de temperatura dos 10 primeiros :\n");

    for (int i = 0; i < total_sensores_temp_global; i++) {      
        double media = sensores_globais[i].soma_temperatura / sensores_globais[i].contagem_temp;
        double media_dos_quadrados = sensores_globais[i].soma_quadrados_temp / sensores_globais[i].contagem_temp;
        double variancia = media_dos_quadrados - (media * media);
        
        if (variancia < 0) variancia = 0; 
        double desvio_padrao = sqrt(variancia);

        if (desvio_padrao > maior_desvio_padrao) {
            maior_desvio_padrao = desvio_padrao;
            strcpy(id_mais_instavel, sensores_globais[i].id);
        }

        if (i < 10) {
            printf("%s:%.2f \n", sensores_globais[i].id, media);
        }
    }

    printf("\nsensor mais instavel e seu desvio padrao:%s  %.2f\n", id_mais_instavel, maior_desvio_padrao);
    printf("total de alertas: %d\n", total_alertas_global);
    printf("consumo total de energia: %.2f\n", consumo_total_energia_global);
    printf("Tempo de execucao: %.4f segundos\n", tempo_execucao);


    return 0;
}