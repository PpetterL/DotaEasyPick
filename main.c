#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <curl/curl.h>
#include "jsmn/jsmn.h"

#define JSON_FILE_PATH "test_matches.json"
#define CACERT "cacert-2022-03-29.pem"
#define JSON_URL "https://api.steampowered.com/IDOTA2Match_570/GetMatchHistoryBySequenceNum/V001/?start_at_match_seq_num=5444708788&game_mode=22&min_players=10&key="

struct string {
  char *ptr;
  size_t len;
};

/*
int str2int(const char* str, int len)
{
    int i;
    int ret = 0;
    for(i = 0; i < len; ++i)
    {
        ret = ret * 10 + (str[i] - '0');
    }
    return ret;
}

unsigned long str2unslong(const char* str, int len)
{
    int i;
    unsigned long ret = 0;
    for(i = 0; i < len; ++i)
    {
        ret = ret * 10 + (str[i] - '0');
    }
    return ret;
}
*/

void init_string(struct string *s) {
  s->len = 0;
  s->ptr = malloc(s->len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "malloc() failed\n");
    exit(EXIT_FAILURE);
  }
  s->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *JSON_STRING)
{
  size_t new_len = JSON_STRING->len + size*nmemb;
  JSON_STRING->ptr = realloc(JSON_STRING->ptr, new_len+1);
  if (JSON_STRING->ptr == NULL) {
    fprintf(stderr, "realloc() failed\n");
    exit(EXIT_FAILURE);
  }
  memcpy(JSON_STRING->ptr+JSON_STRING->len, ptr, size*nmemb);
  JSON_STRING->ptr[new_len] = '\0';
  JSON_STRING->len = new_len;

  return size*nmemb;
}

int apireq(char *URL, struct string *apicontent){
    CURL *curl;
    CURLcode response;
    char buffer[CURL_ERROR_SIZE + 1];
    curl_global_init(CURL_GLOBAL_ALL);

    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_CAINFO, CACERT);
        curl_easy_setopt(curl, CURLOPT_URL, URL);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, apicontent);
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, buffer);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 2);

        /*
        curl_version_info_data *ver = curl_version_info(CURLVERSION_NOW);
        printf("libcurl features %u\n",ver->features);

        */

        response = curl_easy_perform(curl);

        if(response != CURLE_OK) {
            printf("Request failed: %s\n", curl_easy_strerror(response));
            printf("Errorbuffer: %s\n", buffer);
            printf("Response code:%d\n", response);
        } else {

        }
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return 0;
}

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
  if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
      strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
    return 0;
  }
  return -1;
}

void readfile(char *filepath, char *fileContent)
{
    
    //Open file and check for error
    FILE *fptr;

    fptr = fopen(filepath,"r");

    if(fptr == NULL)
    {
        printf("Error! Konnte Datei %s nicht öffnen!", filepath);
        exit(1);
    }

    // Count Lines
    /*
    char cr;
    size_t lines = 0;

    while( cr != EOF ) {
    if ( cr == '\n' ) {
      lines++;
    }
    cr = getc(fptr);
    }
    rewind(fptr);
    */

    // Old version with fgetc
    
    int c;
    long index = 0;

    while((c = fgetc(fptr)) != EOF){
        fileContent[index] = c;
        index++;
    }
    fileContent[index] = '\0';
    

    /*
    while(fgets(buffer, size, fptr)){
        
    }

    printf("Erfolg!\n");

    */

    fclose(fptr);

}

void mycallback(int keylen, char *key, int valuelen, char *value){

    FILE *fptr;

    fptr = fopen("test_matches_parsed.json","a");

    if(fptr == NULL)
    {
        printf("Error! Konnte Datei test_matches_parsed.json nicht öffnen!");
        exit(2);
    }

    if(keylen != 0){
        fprintf(fptr, "\"%.*s\": %.*s,\n", keylen, key, valuelen, value);
    } else {
        fprintf(fptr, "%.*s\n", valuelen, value);
    }
    fclose(fptr);
}

int parseJSON_old(char *URL, char *filepath, void callback(int, char *, int, char *)){
    
    // char JSON_STRING[2097152L];

    struct string JSON_STRING;
    init_string(&JSON_STRING);

    // readtestfile(filepath, JSON_STRING.ptr);
    apireq(URL, &JSON_STRING);

    long i;
    long j;
    long resultCode;
    long matchStart;
    long matchEnd;
    int lobby_type = 0;
    int game_mode = 0;
    jsmn_parser p;
    jsmntok_t t[326144L]; // a number >= total number of tokens

    jsmn_init(&p);
    resultCode = jsmn_parse(&p, JSON_STRING.ptr, JSON_STRING.len, t, sizeof(t)/(sizeof(t[0])));

    if (resultCode < 0) {
       printf("Failed to parse JSON: %ld\n", resultCode);
       return 1;
   }

   /* Assume the top-level element is an object */
   if (resultCode < 1 || t[0].type != JSMN_OBJECT) {
       printf("Object expected\n");
       return 1;
   }

    
    for (i = 1; i < resultCode; i++) {
        if (jsoneq(JSON_STRING.ptr, &t[i], "players") == 0) {
        matchStart = i;
        lobby_type = 0;
        game_mode = 0;

        } else if (jsoneq(JSON_STRING.ptr, &t[i], "lobby_type") == 0) {
            if(strncmp(JSON_STRING.ptr + t[i + 1].start, "7", t[i + 1].end - t[i + 1].start) == 0){
                lobby_type = 7;
                
        }
        matchEnd = i;
        
        } else if (jsoneq(JSON_STRING.ptr, &t[i], "game_mode") == 0) {
            if(strncmp(JSON_STRING.ptr + t[i + 1].start, "22", t[i + 1].end - t[i + 1].start) == 0){
                game_mode = 22;
                
        }
        
        } else if (lobby_type == 7 && game_mode == 22){

            callback(0, "", 1, "{");

            for (j = matchStart; j < matchEnd; j++) {
                if (jsoneq(JSON_STRING.ptr, &t[j], "hero_id") == 0) {
                callback(t[j].end - t[j].start,
                        JSON_STRING.ptr + t[j].start, t[j + 1].end - t[j + 1].start,
                        JSON_STRING.ptr + t[j + 1].start);
                
                } else if (jsoneq(JSON_STRING.ptr, &t[j], "radiant_win") == 0) {
                callback(t[j].end - t[j].start,
                        JSON_STRING.ptr + t[j].start, t[j + 1].end - t[j + 1].start,
                        JSON_STRING.ptr + t[j + 1].start);
                
                } else if (jsoneq(JSON_STRING.ptr, &t[j], "match_seq_num") == 0) {
                callback(t[j].end - t[j].start,
                        JSON_STRING.ptr + t[j].start, t[j + 1].end - t[j + 1].start,
                        JSON_STRING.ptr + t[j + 1].start);
                
                } else {
                
                }
            }
            lobby_type = 0;
            game_mode = 0;
            callback(0, "", 1, "},");
        
        } else {
        
        }
    }
    free(JSON_STRING.ptr);
    return 0;

}

char URL[200] = "";

int parseJSON_oldFile(unsigned long (*oldData)[150], char *filepath){
    
    // char JSON_STRING[2097152L];
    
    struct string JSON_STRING;
    JSON_STRING.len = 262144;
    JSON_STRING.ptr = malloc(8 * JSON_STRING.len);
    if (JSON_STRING.ptr == NULL) {
        fprintf(stderr, "malloc() failed\n");
        exit(EXIT_FAILURE);
    }

    readfile(filepath, JSON_STRING.ptr);

    long i;
    int index;
    int line = -1;
    long resultCode;
    jsmn_parser p;
    jsmntok_t t[32768L]; // a number >= total number of tokens

    jsmn_init(&p);
    resultCode = jsmn_parse(&p, JSON_STRING.ptr, JSON_STRING.len, t, sizeof(t)/(sizeof(t[0])));

    if (resultCode < 0) {
       printf("Failed to parse JSON: %ld\n", resultCode);
       return 1;
   }

   /* Assume the top-level element is an object */
   if (resultCode < 1 || t[0].type != JSMN_OBJECT) {
       printf("Object expected\n");
       return 1;
   }

    
    for (i = 0; i < resultCode; i++) {
        if (t[i].type == JSMN_PRIMITIVE) {
            
            oldData[line][index] = strtol(JSON_STRING.ptr + t[i].start, NULL, 10);
            
            index++;
        } else if (t[i].type == JSMN_ARRAY) {
            line++;
            index = 0;
        } else {
        
        }
    }
    free(JSON_STRING.ptr);
    return 0;

}

void updateData(int *radiHero, int *direHero, int winningTeam, unsigned long (*newDataMatches)[150], unsigned long (*newDataWins)[150]){
    int i;
    int j;
    for (i = 0; i < 5; i++){
        for (j = 0; j < 5; j++){
            if(radiHero[i] < direHero[j]){
                newDataMatches[radiHero[i]][direHero[j]] += 1;
                if(winningTeam == 1){
                    newDataWins[radiHero[i]][direHero[j]] += 1;
                }
            } else {
                newDataMatches[direHero[j]][radiHero[i]] += 1;
                if(winningTeam == 0){
                    newDataWins[direHero[j]][radiHero[i]] += 1;
                }
            }
        }
    }
}

void API_URL(char *api_key){
    char match_seq_num_old[12];
    readfile("match_seq_num.txt", match_seq_num_old);
    strcat(strcat(strcat(strcpy(URL, "https://api.steampowered.com/IDOTA2Match_570/GetMatchHistoryBySequenceNum/V001/?start_at_match_seq_num="), match_seq_num_old), "&game_mode=22&min_players=10&key="), api_key);
}

int parseJSON_API(char *URL, char *filepath, unsigned long long *match_seq_num, unsigned long (*newDataMatches)[150], unsigned long (*newDataWins)[150]){
    
    // char JSON_STRING[2097152L];

    struct string JSON_STRING;
    init_string(&JSON_STRING);

    // readtestfile(filepath, JSON_STRING.ptr);
    apireq(URL, &JSON_STRING);

    long i;
    long j;
    long resultCode;
    long matchStart;
    long matchEnd;
    int lobby_type = 0;
    int game_mode = 0;
    int heroPos = 0;
    int radiHero[5] = {0};
    int direHero[5] = {0};
    int winningTeam = 0;
    jsmn_parser p;
    jsmntok_t t[326144L]; // a number >= total number of tokens

    jsmn_init(&p);
    resultCode = jsmn_parse(&p, JSON_STRING.ptr, JSON_STRING.len, t, sizeof(t)/(sizeof(t[0])));

    if (resultCode < 0) {
       printf("Failed to parse JSON: %ld\n", resultCode);
       return 1;
   }

   /* Assume the top-level element is an object */
   if (resultCode < 1 || t[0].type != JSMN_OBJECT) {
       printf("Object expected\n");
       return 1;
   }

    
    for (i = 1; i < resultCode; i++) {
        if (jsoneq(JSON_STRING.ptr, &t[i], "players") == 0) {
        matchStart = i;
        lobby_type = 0;
        game_mode = 0;
        heroPos = 0;

        } else if (jsoneq(JSON_STRING.ptr, &t[i], "lobby_type") == 0) {
            if(strncmp(JSON_STRING.ptr + t[i + 1].start, "7", t[i + 1].end - t[i + 1].start) == 0){
                lobby_type = 7;
                
        }
        matchEnd = i;
        
        } else if (jsoneq(JSON_STRING.ptr, &t[i], "game_mode") == 0) {
            if(strncmp(JSON_STRING.ptr + t[i + 1].start, "22", t[i + 1].end - t[i + 1].start) == 0){
                game_mode = 22;
                
        }
        
        } else if (lobby_type == 7 && game_mode == 22){

            for (j = matchStart; j < matchEnd; j++) {
                if (jsoneq(JSON_STRING.ptr, &t[j], "hero_id") == 0) {
                    if (heroPos <= 4){
                        radiHero[heroPos++] = strtol(JSON_STRING.ptr + t[j + 1].start, NULL, 10);
                    } else if (heroPos >=5){
                        direHero[heroPos++ - 5] = strtol(JSON_STRING.ptr + t[j + 1].start, NULL, 10);
                    }
                
                } else if (jsoneq(JSON_STRING.ptr, &t[j], "radiant_win") == 0) {
                    if (jsoneq(JSON_STRING.ptr, &t[j + 1], "true") == 0){
                        winningTeam = 1;
                    } else {
                        winningTeam = 0;
                    }
                
                } else if (jsoneq(JSON_STRING.ptr, &t[j], "match_seq_num") == 0) {
                    updateData(radiHero, direHero, winningTeam, newDataMatches, newDataWins);
                    heroPos = 0;
                    *match_seq_num = strtoull(JSON_STRING.ptr + t[j + 1].start, NULL, 10);
                } else {
                
                }
            }
            lobby_type = 0;
            game_mode = 0;
        
        } else {
        
        }
    }
    free(JSON_STRING.ptr);
    return 0;

}

int updateFiles(unsigned long (*oldDataMatches)[150], unsigned long (*oldDataWins)[150], unsigned long (*newDataMatches)[150], unsigned long (*newDataWins)[150], unsigned long long match_seq_num){
    int i;
    int j;
    for(i = 0; i < 150; i++){
        for(j = 0; j < 150; j++){
            newDataMatches[i][j] += oldDataMatches[i][j];
            newDataWins[i][j] += oldDataWins[i][j];
        }
    }
    
    FILE *fptr;

    fptr = fopen("Matches.json","w");

    if(fptr == NULL)
    {
        printf("Error! Konnte Datei Matches.json nicht öffnen!");
        exit(2);
    }

    fprintf(fptr, "{\n");
    for(i = 0; i < 150; i++){
        fprintf(fptr, "[");
        for(j = 0; j < 150; j++){
            if(j < 149){
                fprintf(fptr, "%lu,", newDataMatches[i][j]);
            } else {
                fprintf(fptr, "%lu", newDataMatches[i][j]);
            }
        }
        fprintf(fptr, "]\n");
    }
    fprintf(fptr, "}");

    fclose(fptr);

    FILE *fptr2;

    fptr2 = fopen("Wins.json","w");

    if(fptr2 == NULL)
    {
        printf("Error! Konnte Datei Wins.json nicht öffnen!");
        exit(2);
    }

    fprintf(fptr2, "{\n");
    for(i = 0; i < 150; i++){
        fprintf(fptr2, "[");
        for(j = 0; j < 150; j++){
            if(j < 149){
                fprintf(fptr2, "%lu,", newDataWins[i][j]);
            } else {
                fprintf(fptr2, "%lu", newDataWins[i][j]);
            }
        }
        fprintf(fptr2, "]\n");
    }
    fprintf(fptr2, "}");
    
    fclose(fptr2);

    FILE *fptr3;

    fptr3 = fopen("Winrates.json","w");

    if(fptr3 == NULL)
    {
        printf("Error! Konnte Datei Winrates.json nicht öffnen!");
        exit(2);
    }

    fprintf(fptr3, "{\n");
    for(i = 0; i < 150; i++){
        fprintf(fptr3, "[");
        for(j = 0; j < 150; j++){
            if(j < 149){
                if (newDataWins[i][j] != 0) {
                    fprintf(fptr3, "%.4f,", (float) newDataWins[i][j] / newDataMatches[i][j]);
                } else if (newDataMatches[i][j] != 0) {
                    fprintf(fptr3, "%.4f,", 1.0);
                } else {
                    fprintf(fptr3, "%.4f,", 0.0);
                }
            } else {
                if(newDataWins[i][j] != 0) {
                    fprintf(fptr3, "%.4f", (float) newDataWins[i][j] / newDataMatches[i][j]);
                } else if (newDataMatches[i][j] != 0) {
                    fprintf(fptr3, "%.4f", 1.0);
                } else {
                    fprintf(fptr3, "%.4f", 0.0);
                }
            }
        }
        fprintf(fptr3, "]\n");
    }
    fprintf(fptr3, "}");
    
    fclose(fptr3);

    if(match_seq_num != 0) {
        FILE *fptr4;

        fptr4 = fopen("match_seq_num.txt","w");

        if(fptr4 == NULL)
        {
            printf("Error! Konnte Datei match_seq_num.txt nicht öffnen!");
            exit(2);
        }

        fprintf(fptr4,"%llu", match_seq_num + 1);

        fclose(fptr4);
    }
    
    return 0;
}

int main(int argc, char **argv){

    char *api_key = argv[1];
    int i = 0;
    for(i = 0; i < 1000; i++){
        unsigned long oldDataMatches[150][150] = {0};
        unsigned long oldDataWins[150][150] = {0};
        unsigned long newDataMatches[150][150] = {0};
        unsigned long newDataWins[150][150] = {0};
        unsigned long long match_seq_num = 0;

        printf("Loop1\n");
        parseJSON_oldFile(oldDataMatches, "Matches.json");
        printf("Loop2\n");
        parseJSON_oldFile(oldDataWins, "Wins.json");
        printf("Loop3\n");
        API_URL(api_key);
        printf("Loop4\n");
        if(parseJSON_API(URL, JSON_FILE_PATH, &match_seq_num, newDataMatches, newDataWins) == 0) {
            printf("Loop5\n");
            updateFiles(oldDataMatches, oldDataWins, newDataMatches, newDataWins, match_seq_num);
            printf("Loop6\n");
        }
        Sleep(3000);
    }
    
    /*
    // parseJSON_old(JSON_URL, JSON_FILE_PATH, mycallback);
    parseJSON_oldFile("Matches.json", 0);
    parseJSON_oldFile("Wins.json", 1);
    API_URL();
    parseJSON_API(URL, JSON_FILE_PATH);
    updateFiles();
    */

    return 0;

}