/* 
 *  libpinyin
 *  Library to deal with pinyin.
 *  
 *  Copyright (C) 2012 Peng Wu <alexepico@gmail.com>
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <assert.h>
#include <string.h>
#include "phrase_large_table2.h"


/* class definition */

namespace pinyin{

class PhraseLengthIndexLevel2{
protected:
    GArray * m_phrase_array_indexes;
public:
    PhraseLengthIndexLevel2();
    ~PhraseLengthIndexLevel2();

    /* load/store method */
    bool load(MemoryChunk * chunk, table_offset_t offset, table_offset_t end);
    bool store(MemoryChunk * new_chunk, table_offset_t offset, table_offset_t & end);

    /* search method */
    int search(int phrase_length, /* in */ ucs4_t phrase[],
               /* out */ PhraseTokens tokens) const;

    /* add_index/remove_index method */
    int add_index(int phrase_length, /* in */ ucs4_t phrase[],
                  /* in */ phrase_token_t token);
    int remove_index(int phrase_length, /* in */ ucs4_t phrase[],
                     /* in */ phrase_token_t token);
};


template<size_t phrase_length>
struct PhraseIndexItem2{
    phrase_token_t m_token;
    ucs4_t m_phrase[phrase_length];
public:
    PhraseIndexItem2<phrase_length>(ucs4_t phrase[], phrase_token_t token){
        memmove(m_phrase, phrase, sizeof(ucs4_t) * phrase_length);
        m_token = token;
    }
};


template<size_t phrase_length>
class PhraseArrayIndexLevel2{
protected:
    typedef PhraseIndexItem2<phrase_length> IndexItem;

protected:
    MemoryChunk m_chunk;
public:
    bool load(MemoryChunk * chunk, table_offset_t offset, table_offset_t end);
    bool store(MemoryChunk * new_chunk, table_offset_t offset, table_offset_t & end);

    /* search method */
    int search(/* in */ ucs4_t phrase[], /* out */ PhraseTokens tokens) const;

    /* add_index/remove_index method */
    int add_index(/* in */ ucs4_t phrase[], /* in */ phrase_token_t token);
    int remove_index(/* in */ ucs4_t phrase[], /* in */ phrase_token_t token);
};

};

using namespace pinyin;

/* class implementation */

template<size_t phrase_length>
static int phrase_compare2(const PhraseIndexItem2<phrase_length> &lhs,
                           const PhraseIndexItem2<phrase_length> &rhs){
    ucs4_t * phrase_lhs = (ucs4_t *) lhs.m_phrase;
    ucs4_t * phrase_rhs = (ucs4_t *) rhs.m_phrase;

    return memcmp(phrase_lhs, phrase_rhs, sizeof(ucs4_t) * phrase_length);
}

template<size_t phrase_length>
static bool phrase_less_than2(const PhraseIndexItem2<phrase_length> & lhs,
                              const PhraseIndexItem2<phrase_length> & rhs){
    return 0 > phrase_compare2(lhs, rhs);
}

PhraseBitmapIndexLevel2::PhraseBitmapIndexLevel2(){
    memset(m_phrase_length_indexes, 0, sizeof(m_phrase_length_indexes));
}

void PhraseBitmapIndexLevel2::reset(){
    for ( size_t i = 0; i < PHRASE_NUMBER_OF_BITMAP_INDEX; i++){
        PhraseLengthIndexLevel2 * length_array =
            m_phrase_length_indexes[i];
        if ( length_array )
            delete length_array;
    }
}

int PhraseBitmapIndexLevel2::search(int phrase_length,
                                    /* in */ ucs4_t phrase[],
                                    /* out */ PhraseTokens tokens) const {
    assert(phrase_length > 0);

    int result = SEARCH_NONE;
    /* use the first 8-bit of the lower 16-bit for bitmap index,
     * as most the higher 16-bit are zero.
     */
    guint8 first_key = (phrase[0] & 0xFF00) >> 8;

    PhraseLengthIndexLevel2 * phrase_array = m_phrase_length_indexes[first_key];
    if ( phrase_array )
        return phrase_array->search(phrase_length, phrase, tokens);
    return result;
}

PhraseLengthIndexLevel2::PhraseLengthIndexLevel2(){
    m_phrase_array_indexes = g_array_new(FALSE, TRUE, sizeof(void *));
}

PhraseLengthIndexLevel2::~PhraseLengthIndexLevel2(){
#define CASE(len) case len:                                             \
    {                                                                   \
        PhraseArrayIndexLevel2<len> * & array =  g_array_index          \
            (m_phrase_array_indexes, PhraseArrayIndexLevel2<len> *, len - 1); \
        if ( array ) {                                                  \
            delete array;                                               \
            array = NULL;                                               \
        }                                                               \
        break;                                                          \
    }

    for (size_t i = 1; i <= m_phrase_array_indexes->len; ++i){
        switch (i){
	    CASE(1);
	    CASE(2);
	    CASE(3);
	    CASE(4);
	    CASE(5);
	    CASE(6);
	    CASE(7);
	    CASE(8);
	    CASE(9);
	    CASE(10);
	    CASE(11);
	    CASE(12);
	    CASE(13);
	    CASE(14);
	    CASE(15);
	    CASE(16);
	default:
	    assert(false);
        }
    }
    g_array_free(m_phrase_array_indexes, TRUE);
#undef CASE
}

int PhraseLengthIndexLevel2::search(int phrase_length,
                                    /* in */ ucs4_t phrase[],
                                    /* out */ PhraseTokens tokens) const {
    int result = SEARCH_NONE;
    if(m_phrase_array_indexes->len < phrase_length)
        return result;
    if (m_phrase_array_indexes->len > phrase_length)
        result |= SEARCH_CONTINUED;

#define CASE(len) case len:                                             \
    {                                                                   \
        PhraseArrayIndexLevel2<len> * array = g_array_index             \
            (m_phrase_array_indexes, PhraseArrayIndexLevel2<len> *, len - 1); \
        if ( !array )                                                   \
            return result;                                              \
        result |= array->search(phrase, tokens);                        \
        return result;                                                  \
    }

    switch ( phrase_length ){
	CASE(1);
	CASE(2);
	CASE(3);
	CASE(4);
	CASE(5);
	CASE(6);
	CASE(7);
	CASE(8);
	CASE(9);
	CASE(10);
	CASE(11);
	CASE(12);
	CASE(13);
	CASE(14);
	CASE(15);
	CASE(16);
    default:
	assert(false);
    }
#undef CASE
}

template<size_t phrase_length>
int PhraseArrayIndexLevel2<phrase_length>::search
(/* in */ ucs4_t phrase[], /* out */ PhraseTokens tokens) const {
    int result = SEARCH_NONE;

    IndexItem * chunk_begin = NULL, * chunk_end = NULL;
    chunk_begin = (IndexItem *) m_chunk.begin();
    chunk_end = (IndexItem *) m_chunk.end();

    /* do the search */
    IndexItem item(phrase, -1);
    std_lite::pair<IndexItem *, IndexItem *> range;
    range = std_lite::equal_range
        (chunk_begin, chunk_end, item,
         phrase_less_than2<phrase_length>);

    const IndexItem * const begin = range.first;
    const IndexItem * const end = range.second;
    if (begin == end)
        return result;

    const IndexItem * iter = NULL;
    GArray * array = NULL;

    for (iter = begin; iter != end; ++iter) {
        phrase_token_t token = iter->m_token;

        /* filter out disabled sub phrase indices. */
        array = tokens[PHRASE_INDEX_LIBRARY_INDEX(token)];
        if (NULL == array)
            continue;

        result |= SEARCH_OK;

        g_array_append_val(array, token);
    }

    return result;
}

int PhraseBitmapIndexLevel2::add_index(int phrase_length,
                                       /* in */ ucs4_t phrase[],
                                       /* in */ phrase_token_t token){
    guint8 first_key =  (phrase[0] & 0xFF00) >> 8;

    PhraseLengthIndexLevel2 * & length_array =
        m_phrase_length_indexes[first_key];

    if ( !length_array ){
        length_array = new PhraseLengthIndexLevel2();
    }
    return length_array->add_index(phrase_length, phrase, token);
}

int PhraseBitmapIndexLevel2::remove_index(int phrase_length,
                                         /* in */ ucs4_t phrase[],
                                         /* in */ phrase_token_t token){
    guint8 first_key = (phrase[0] & 0xFF00) >> 8;

    PhraseLengthIndexLevel2 * & length_array =
        m_phrase_length_indexes[first_key];

    if ( length_array )
        return length_array->remove_index(phrase_length, phrase, token);

    return ERROR_REMOVE_ITEM_DONOT_EXISTS;
}

int PhraseLengthIndexLevel2::add_index(int phrase_length,
                                       /* in */ ucs4_t phrase[],
                                       /* in */ phrase_token_t token) {
    if (phrase_length >= MAX_PHRASE_LENGTH)
        return ERROR_PHRASE_TOO_LONG;

    if (m_phrase_array_indexes->len < phrase_length)
        g_array_set_size(m_phrase_array_indexes, phrase_length);

#define CASE(len) case len:                                             \
    {                                                                   \
        PhraseArrayIndexLevel2<len> * & array = g_array_index           \
            (m_phrase_array_indexes, PhraseArrayIndexLevel2<len> *, len - 1); \
        if ( !array )                                                   \
            array = new PhraseArrayIndexLevel2<len>;                    \
        return array->add_index(phrase, token);                         \
    }

    switch(phrase_length){
	CASE(1);
	CASE(2);
	CASE(3);
	CASE(4);
	CASE(5);
	CASE(6);
	CASE(7);
	CASE(8);
	CASE(9);
	CASE(10);
	CASE(11);
	CASE(12);
	CASE(13);
	CASE(14);
	CASE(15);
        CASE(16);
    default:
	assert(false);
    }

#undef CASE
}

int PhraseLengthIndexLevel2::remove_index(int phrase_length,
                                          /* in */ ucs4_t phrase[],
                                          /* in */ phrase_token_t token) {
    if (phrase_length >= MAX_PHRASE_LENGTH)
        return ERROR_PHRASE_TOO_LONG;

    if (m_phrase_array_indexes->len < phrase_length)
        return ERROR_REMOVE_ITEM_DONOT_EXISTS;

#define CASE(len) case len:                                             \
    {                                                                   \
        PhraseArrayIndexLevel2<len> * & array =  g_array_index          \
            (m_phrase_array_indexes, PhraseArrayIndexLevel2<len> *, len - 1); \
        if ( !array )                                                   \
            return ERROR_REMOVE_ITEM_DONOT_EXISTS;                      \
        return array->remove_index(phrase, token);                      \
    }

    switch(phrase_length){
	CASE(1);
	CASE(2);
	CASE(3);
	CASE(4);
	CASE(5);
	CASE(6);
	CASE(7);
	CASE(8);
	CASE(9);
	CASE(10);
	CASE(11);
	CASE(12);
	CASE(13);
	CASE(14);
	CASE(15);
	CASE(16);
    default:
	assert(false);
    }
#undef CASE
}
