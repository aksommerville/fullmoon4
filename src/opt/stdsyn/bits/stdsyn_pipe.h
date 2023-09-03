/* stdsyn_pipe.h
 * Generic composable pipeline of nodes.
 */
 
#ifndef STDSYN_PIPE_H
#define STDSYN_PIPE_H

struct stdsyn_pipe_config;
struct stdsyn_pipe;

struct stdsyn_pipe_config {
  struct stdsyn_pipe_config_cmd {
    const struct stdsyn_node_type *type;
    uint8_t bufid;
    uint8_t overwrite;
    void *argv;
    int argc;
  } *cmdv;
  int cmdc,cmda;
};

struct stdsyn_pipe {
  int defunct; // 0=running, 1=defunct, <0=no envelope
  struct stdsyn_pipe_cmd {
    struct stdsyn_node *node;
    uint8_t bufid;
  } *cmdv;
  int cmdc,cmda;
};

void stdsyn_pipe_config_cleanup(struct stdsyn_pipe_config *config);
int stdsyn_pipe_config_copy(struct stdsyn_pipe_config *dst,const struct stdsyn_pipe_config *src);

struct stdsyn_pipe_config_cmd *stdsyn_pipe_config_add_command(struct stdsyn_pipe_config *config);

void stdsyn_pipe_cleanup(struct stdsyn_pipe *pipe);

int stdsyn_pipe_init(
  struct stdsyn_pipe *pipe,
  const struct stdsyn_pipe_config *config,
  struct bigpc_synth_driver *driver,
  uint8_t noteid,uint8_t velocity
);

void stdsyn_pipe_update(struct stdsyn_pipe *pipe,int c);

void stdsyn_pipe_release(struct stdsyn_pipe *pipe,uint8_t velocity);

#endif
