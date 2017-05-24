/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "msg/msg.h"            /* Yeah! If you want to use msg, you need to include msg/msg.h */
#include "xbt/sysdep.h"         /* calloc, printf */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,
                             "Messages specific for this msg example");

int communicator_size = 0;
typedef struct {
  int last_Irecv_sender_id;
  int bcast_counter;
  int reduce_counter;
  int allReduce_counter;
  xbt_dynar_t isends;           /* of msg_comm_t */
  /* Used to implement irecv+wait */
  xbt_dynar_t irecvs;           /* of msg_comm_t */
  xbt_dynar_t tasks;            /* of msg_task_t */
} s_process_globals_t, *process_globals_t;

// #define ACT_DEBUG(...) \
//   if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose)) {  \
//     char *NAME = xbt_str_join_array(action, " ");              \
//     XBT_DEBUG(__VA_ARGS__);                                    \
//     xbt_free(NAME);                                            \
//   } else ((void)0)


/* Helper function */
static double parse_double(const char *string)
{
  double value;
  char *endptr;

  value = strtod(string, &endptr);
  if (*endptr != '\0')
    THROWF(unknown_error, 0, "%s is not a double", string);
  return value;
}

typedef struct task_data {

  int value;

} s_task_data, *task_data;

int master(int argc, char *argv[]);
int worker(int argc, char *argv[]);
msg_error_t test_all(const char *platform_file,
                     const char *application_file,
                     char *action_file);

#define FINALIZE ((void*)221297)        /* a magic number to tell people to stop working */

/** Emitter function  */
int master(int argc, char *argv[])
{
  int workers_count = 0;
  msg_host_t *workers = NULL;
  msg_task_t *todo = NULL;
  int number_of_tasks = 0;
  double task_comp_size = 0;
  double task_comm_size = 0;

  int i;

  _XBT_GNUC_UNUSED int res = sscanf(argv[1], "%d", &number_of_tasks);
  xbt_assert(res,"Invalid argument %s\n", argv[1]);
  res = sscanf(argv[2], "%lg", &task_comp_size);
  xbt_assert(res, "Invalid argument %s\n", argv[2]);
  res = sscanf(argv[3], "%lg", &task_comm_size);
  xbt_assert(res, "Invalid argument %s\n", argv[3]);

  {                             /*  Task creation */
    char sprintf_buffer[64];

    todo = xbt_new0(msg_task_t, number_of_tasks);

    for (i = 0; i < number_of_tasks; i++) {
      sprintf(sprintf_buffer, "Task_%d", i);

      task_data data = xbt_new(s_task_data, 1);
      data->value = 5;
      todo[i] = MSG_task_create(sprintf_buffer, task_comp_size, task_comm_size, data);

    }
  }

  {                             /* Process organisation */
    workers_count = argc - 4;
    workers = xbt_new0(msg_host_t, workers_count);

    for (i = 4; i < argc; i++) {
      workers[i - 4] = MSG_get_host_by_name(argv[i]);
      xbt_assert(workers[i - 4] != NULL, "Unknown host %s. Stopping Now! ",
                  argv[i]);
    }
  }

  XBT_INFO("Got %d workers and %d tasks to process", workers_count,
        number_of_tasks);


  xbt_dynar_t host_list;
  msg_host_t host;
  host_list = MSG_hosts_as_dynar();
  const char *myName = MSG_host_get_name(MSG_host_self());

  xbt_dynar_foreach(host_list, i, host) {
    const char *descr = MSG_host_get_name(host);
//    MSG_host_set_property_value(host, "stream", (char*)stream, NULL);

    if (strncmp(descr, myName, 5) != 0) {
      XBT_INFO("Sending \"%s\" to \"%s\"", todo[i]->name, descr);
      MSG_task_send(todo[i], descr);
      XBT_INFO("Sent");	
    }

  }

/*
  for (i = 0; i < number_of_tasks; i++) {
    XBT_INFO("Sending \"%s\" to \"%s\"",
          todo[i]->name, MSG_host_get_name(workers[i % workers_count]));
    
    MSG_task_send(todo[i], MSG_host_get_name(workers[i % workers_count]));
    XBT_INFO("Sent");
  }
*/
  XBT_INFO
      ("All tasks have been dispatched. Let's tell everybody the computation is over.");
  for (i = 0; i < workers_count; i++) {
    msg_task_t finalize = MSG_task_create("finalize", 0, 0, FINALIZE);
    MSG_task_send(finalize, MSG_host_get_name(workers[i]));
  }

  XBT_INFO("Goodbye now!");
  free(workers);
  free(todo);
  return 0;
}                               /* end_of_master */

static void log_action(const char *const *action, double date)
{
  // if (XBT_LOG_ISENABLED(actions, xbt_log_priority_verbose)) {
    char *name = xbt_str_join_array(action, " ");
    XBT_INFO("%s %f", name, date);
    xbt_free(name);
  // }
}

void broadcast_message(const char *const *action) {
    int i;
  char *bcast_identifier;
  char mailbox[80];
  double comm_size = parse_double(action[2]);
  msg_task_t task = NULL;
  const char *process_name;
  double clock = MSG_get_clock();

  process_globals_t counters =
      (process_globals_t) MSG_process_get_data(MSG_process_self());

  xbt_assert(communicator_size, "Size of Communicator is not defined, "
             "can't use collective operations");

  process_name = MSG_process_get_name(MSG_process_self());

  bcast_identifier = bprintf("bcast_%d", counters->bcast_counter++);

  if (!strcmp(process_name, "p0")) {
    XBT_DEBUG("%s: %s is the Root", bcast_identifier, process_name);

    msg_comm_t *comms = xbt_new0(msg_comm_t, communicator_size - 1);

    for (i = 1; i < communicator_size; i++) {
      sprintf(mailbox, "%s_p0_p%d", bcast_identifier, i);
      comms[i - 1] =
          MSG_task_isend(MSG_task_create(mailbox, 0, comm_size, NULL), mailbox);
    }
    MSG_comm_waitall(comms, communicator_size - 1, -1);
    for (i = 1; i < communicator_size; i++)
      MSG_comm_destroy(comms[i - 1]);
    xbt_free(comms);

    XBT_DEBUG("%s: all messages sent by %s have been received",
              bcast_identifier, process_name);

  } else {
    sprintf(mailbox, "%s_p0_%s", bcast_identifier, process_name);
    MSG_task_receive(&task, mailbox);
    MSG_task_destroy(task);
    XBT_DEBUG("%s: %s has received", bcast_identifier, process_name);
  }

  log_action(action, MSG_get_clock() - clock);
  xbt_free(bcast_identifier);
}



/** Receiver function  */
int worker(int argc, char *argv[])
{
  msg_task_t task = NULL;
  _XBT_GNUC_UNUSED int res;
  int i;


  while (1) {
    res = MSG_task_receive(&(task),MSG_host_get_name(MSG_host_self()));
    xbt_assert(res == MSG_OK, "MSG_task_receive failed");

    XBT_INFO("Received \"%s\"", MSG_task_get_name(task));

    task_data data = MSG_task_get_data(task);
    XBT_INFO("Data is : %d", data->value);
    if (!strcmp(MSG_task_get_name(task), "finalize")) 
    {
      MSG_task_destroy(task);
      break;
    }

   // XBT_INFO("Processing \"%s\"", MSG_task_get_name(task));
   // MSG_task_execute(task);
   // XBT_INFO("\"%s\" done", MSG_task_get_name(task));

//send any received msg to all processes
  xbt_dynar_t host_list;
  msg_host_t host;
  host_list = MSG_hosts_as_dynar();
  const char *myName = MSG_host_get_name(MSG_host_self());


if (!strcmp(process_name, "p0")) {
    XBT_DEBUG("%s: %s is the Root", bcast_identifier, process_name);

    msg_comm_t *comms = xbt_new0(msg_comm_t, communicator_size - 1);

    for (i = 1; i < communicator_size; i++) {
      sprintf(mailbox, "%s_p0_p%d", bcast_identifier, i);
      comms[i - 1] =
          MSG_task_isend(MSG_task_create(mailbox, 0, comm_size, NULL), mailbox);
    }
    MSG_comm_waitall(comms, communicator_size - 1, -1);
    for (i = 1; i < communicator_size; i++)
      MSG_comm_destroy(comms[i - 1]);
    xbt_free(comms);

    XBT_DEBUG("%s: all messages sent by %s have been received",
              bcast_identifier, process_name);

  } else {
    sprintf(mailbox, "%s_p0_%s", bcast_identifier, process_name);
    MSG_task_receive(&task, mailbox);
    MSG_task_destroy(task);
    XBT_DEBUG("%s: %s has received", bcast_identifier, process_name);
  }


  xbt_dynar_foreach(host_list, i, host) {
    const char *descr = MSG_host_get_name(host);
//    MSG_host_set_property_value(host, "stream", (char*)stream, NULL);

    if (strncmp(descr, "host0", 5) != 0 && strncmp(descr, myName, 5) != 0) {
      XBT_INFO("Sending \"%s\" to \"%s\"", task->name, descr);
      MSG_task_send(task, descr);
      XBT_INFO("Sent"); 
    } else {
      XBT_INFO("+++NOT SENT TO %s", descr);
    }

  }


    MSG_task_destroy(task);
    task = NULL;
    break;
  }
  XBT_INFO("I'm done. See you!");
  return 0;
}                               /* end_of_worker */

/** Test function */
msg_error_t test_all(const char *platform_file,
                     const char *application_file, 
                     char *action_file)
{
  msg_error_t res = MSG_OK;

  {                             /*  Simulation setting */
    MSG_create_environment(platform_file);
  }
  {                             /*   Application deployment */
    MSG_function_register("master", master);
    MSG_function_register("worker", worker);


    MSG_launch_application(application_file);

    //xbt_replay_action_register("reduce", action_reduce);

    MSG_action_trace_run(action_file);
  }
  res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());
  return res;
}                               /* end_of_test_all */


/** Main function */
int main(int argc, char *argv[])
{
  msg_error_t res = MSG_OK;

  MSG_init(&argc, argv);
  if (argc < 3) {
    printf("Usage: %s platform_file deployment_file\n", argv[0]);
    printf("example: %s msg_platform.xml msg_deployment.xml\n", argv[0]);
    exit(1);
  }
  res = test_all(argv[1], argv[2], argv[3]);

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}                               /* end_of_main */
