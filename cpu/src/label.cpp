#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#include "../../lib/logs/log.h"
#include "../../lib/read_write/read_write.h"
#include "../../lib/algorithm/algorithm.h"
#include "../../lib/stack/stack.h"

#include "label.h"

/*===========================================================================================================================*/
// LABEL_STORE_CTOR_DTOR
/*===========================================================================================================================*/

void label_store_ctor(label_store *const link)
{
    assert(link != nullptr);

    link->store    = nullptr;
    link->size     = 0;
    link->capacity = 0;
}

bool label_store_ctor(label_store *const link, const int capacity)
{
    assert(link     != nullptr);
    assert(capacity >=       0);

    link->store    = (label *) log_calloc((size_t) capacity, sizeof(label));
    link->capacity = capacity;
    link->size     = 0;
    
    if (link->store == nullptr && capacity != 0)
    {
        log_error(        "can't allocate memory for label array(%d)\n", __LINE__);
        fprintf  (stderr, "can't allocate memory for label array\n");
        return false;
    }
    return true;
}

void label_store_dtor(label_store *const link)
{
    assert(link != nullptr);

    log_free(link->store);
}

/*===========================================================================================================================*/
// LABEL_STORE_PUSH
/*===========================================================================================================================*/

void label_store_push(label_store *const link, const int token_num, const int label_pc)
{
    assert(link != nullptr);
    assert(link->size < link->capacity);

    link->store[link->size].token_num = token_num;
    link->store[link->size].pc        = label_pc;

    link->size++;
}
