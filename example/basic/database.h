#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../../src/logger.h"

#define MAX_ROW_COUNT 1024
#define MAX_NAME_LEN 32

typedef struct UserTable {
    uint16_t row_count;
    uint16_t id[MAX_ROW_COUNT];
    uint8_t age[MAX_ROW_COUNT];
    char first_name[MAX_NAME_LEN][MAX_ROW_COUNT];
    char last_name[MAX_NAME_LEN][MAX_ROW_COUNT];
} UserTable;

typedef struct User {
    uint16_t id;
    int8_t age;
    char first_name[MAX_NAME_LEN];
    char last_name[MAX_NAME_LEN];
} User;

int UserTable_init(UserTable* t) {
    t->row_count = 0;
    return 0;
}

int UserTable_add(UserTable* t, User user) {
    if (t->row_count >= MAX_ROW_COUNT) {
        printf("Unable to add. Table is full.");
        return -1;
    }

    if (strlen(user.first_name) == 0 && strlen(user.last_name) == 0) {
        printf("Cannot have both first_name and last_name empty.");
        return -1;
    }

    for (uint16_t i=0; i<t->row_count; i++) {
        if (strcmp(t->first_name[i], user.first_name) == 0 && strcmp(t->last_name[i], user.last_name) == 0) {
            ERROR("User '%s %s' already exist.", user.first_name, user.last_name);
            return -1;
        }
    }

    uint16_t index = t->row_count;
    t->id[index] = index == 0 ? 0 : t->id[index-1] + 1;
    t->age[index] = user.age;
    sprintf(t->first_name[index], "%s", user.first_name);
    sprintf(t->last_name[index], "%s", user.last_name);

    t->row_count++;

    return 0;
}

User UserTable_get_user_by_name(UserTable* t, char* first_name, char* last_name) {
    User ret;
    ret.id = (uint16_t)-1;
    for (uint16_t i=0; i<t->row_count; i++) {
        if (strcmp(first_name, t->first_name[i]) == 0 && strcmp(last_name, t->last_name[i]) == 0) {
            DEBUG("User '%s %s' found.", first_name, last_name);
            if (strlen(t->first_name[i]) >= MAX_NAME_LEN || strlen(t->last_name[i]) > MAX_NAME_LEN) {
                ERROR("Name too long. Only %d characters allowed.", MAX_NAME_LEN);
                return ret;
            }
            
            ret.id = t->id[i];
            ret.age = t->age[i];
            sprintf(ret.first_name, "%s", t->first_name[i]);
            sprintf(ret.last_name, "%s", t->last_name[i]);
            break;
        }
    }
    return ret;
}

int UserTable_update(UserTable* t, User user) {
    if (strlen(user.first_name) >= MAX_NAME_LEN || strlen(user.last_name) > MAX_NAME_LEN) {
        ERROR("Name too long. Only %d characters allowed.", MAX_NAME_LEN);
        return -1;
    }

    if (strlen(user.first_name) == 0 && strlen(user.last_name) == 0) {
        ERROR("Cannot have both first_name and last_name empty.");
        return -1;
    }

    if (user.id == (uint16_t)-1) {
        user.id = UserTable_get_user_by_name(t, user.first_name, user.last_name).id;
        if (user.id == (uint16_t)-1) {
            ERROR("Unable to find user '%s %s'.", user.first_name, user.last_name);
            return -1;
        }

    }

    for (uint16_t i=0; i<t->row_count; i++) {
        if (user.id == t->id[i]) {
            DEBUG("User '%s %s' (%d) found.", user.first_name, user.last_name, user.id);
            t->age[i] = user.age;
            sprintf(t->first_name[i], "%s", user.first_name);
            sprintf(t->last_name[i], "%s", user.last_name);
            break;
        }
    }

    return 0;
}

int UserTable_delete(UserTable* t, User user) {
    if (user.id == (uint16_t)-1) {
        user.id = UserTable_get_user_by_name(t, user.first_name, user.last_name).id;
        if (user.id == (uint16_t)-1) {
            ERROR("Unable to find user '%s %s'.", user.first_name, user.last_name);
            return -1;
        }

    }

    for (uint16_t i=0; i<t->row_count; i++) {
        if (user.id == t->id[i]) {
            if (i < t->row_count - 1) {
                t->age[i] = t->age[t->row_count-1];
                sprintf(t->first_name[i], "%s", t->first_name[t->row_count-1]);
                sprintf(t->last_name[i], "%s", t->last_name[t->row_count-1]);
            }  
            t->row_count--;
            break;
        }
    }

    return 0;
}