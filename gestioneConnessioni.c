#include "gestioneConnessioni.h"

char *get_tipo_richiesta(char *messaggio){

    //parse JSON del messaggio
    struct json_object *parsed_json;
    parsed_json = json_tokener_parse(messaggio);

    //Estrazione tipo di richiesta
    struct json_object *tipoRichiesta;
    json_object_object_get_ex(parsed_json, "tipoRichiesta", &tipoRichiesta);

    //Return stringra della richiesta
    return json_object_get_string(tipoRichiesta);

}
