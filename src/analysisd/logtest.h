/* Copyright (C) 2015-2020, Wazuh Inc.
 * All right reserved.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */

#include "shared.h"
#include "rules.h"
#include "config.h"
#include "decoders/decoder.h"
#include "eventinfo.h"
#include "lists.h"
#include "lists_make.h"
#include "fts.h"
#include "accumulator.h"
#include "../config/logtest-config.h"
#include "../os_net/os_net.h"
#include <time.h>


/**
 * @brief A w_logtest_session_t instance represents a client
 */
typedef struct w_logtest_session_t {

    char *token;                            ///< Client ID
    time_t last_connection;                 ///< Timestamp of the last query

    RuleNode *rule_list;                    ///< Rule list
    OSDecoderNode *decoderlist_forpname;    ///< Decoder list to match logs which have a program name
    OSDecoderNode *decoderlist_nopname;     ///< Decoder list to match logs which haven't a program name
    OSStore *decoder_store;                  ///< Decoder list to save internals decoders
    ListNode *cdblistnode;                  ///< List of CDB lists
    ListRule *cdblistrule;                  ///< List to attach rules and CDB lists
    EventList *eventlist;                   ///< Previous events list
    OSHash *g_rules_hash;                   ///< Hash table of rules
    OSList *fts_list;                       ///< Save FTS previous events
    OSHash *fts_store;                      ///< Save FTS values processed
    OSHash *acm_store;                      ///< Hash to save data which have the same id
    int acm_lookups;                        ///< Counter of the number of times purged. Option accumulate
    time_t acm_purge_ts;                    ///< Counter of the time interval of last purge. Option accumulate

} w_logtest_session_t;

/**
 * @brief List of client actives
 */
OSHash *w_logtest_sessions;

/**
 * @brief An instance of w_logtest_connection allow managing the connections with the logtest socket
 */
typedef struct w_logtest_connection_t {

    pthread_mutex_t mutex;      ///< Mutex to prevent race condition in accept syscall
    int sock;                   ///< The open connection with logtest queue

} w_logtest_connection_t;


/**
 * @brief Initialize Wazuh Logtest. Initialize the listener and create threads
 * Then, call function w_logtest_main
 */
void *w_logtest_init();

/**
 * @brief Initialize logtest configuration. Then, call ReadConfig
 *
 * @return OS_SUCCESS on success, otherwise OS_INVALID
 */
int w_logtest_init_parameters();

/**
 * @brief Main function of Wazuh Logtest module
 *
 * Listen and treat connections with clients
 *
 * @param connection The listener where clients connect
 */
void *w_logtest_main(w_logtest_connection_t * connection);

/**
 * @brief Process client's request
 * @param fd File descriptor which represents the client
 */
void w_logtest_process_log(char * token);

/**
 * @brief Create resources necessary to service client
 * @param fd File descriptor which represents the client
 */
w_logtest_session_t *w_logtest_initialize_session(char * token, OSList* list_msg);

/**
 * @brief Free resources after client closes connection
 * @param fd File descriptor which represents the client
 */
void w_logtest_remove_session(char * token);

/**
 * @brief Check the inactive logtest sessions
 *
 * Check all the sessions. If a session has been inactive longer than session_timeout,
 * call w_logtest_remove_session to remove it.
 */
void * w_logtest_check_inactive_sessions(__attribute__((unused)) void * arg);

/**
 * @brief Initialize FTS engine for a client session
 * @param fts_list list which save fts previous events
 * @param fts_store hash table which save fts values processed previously
 * @return 1 on success, otherwise return 0
 */
int w_logtest_fts_init(OSList **fts_list, OSHash **fts_store);
