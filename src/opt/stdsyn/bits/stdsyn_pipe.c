#include "../fmn_stdsyn_internal.h"
#include "stdsyn_pipe.h"

/* Config cleanup.
 */
 
static void stdsyn_pipe_config_cmd_cleanup(struct stdsyn_pipe_config_cmd *cmd) {
  if (cmd->argv) free(cmd->argv);
}

void stdsyn_pipe_config_cleanup(struct stdsyn_pipe_config *config) {
  if (config->cmdv) {
    while (config->cmdc-->0) stdsyn_pipe_config_cmd_cleanup(config->cmdv+config->cmdc);
    free(config->cmdv);
  }
  memset(config,0,sizeof(struct stdsyn_pipe_config));
}

/* Config copy.
 */
 
int stdsyn_pipe_config_copy(struct stdsyn_pipe_config *dst,const struct stdsyn_pipe_config *src) {
  if (!dst||!src) return -1;
  memset(dst,0,sizeof(struct stdsyn_pipe_config));
  if (src->cmdc) {
    if (!(dst->cmdv=malloc(sizeof(struct stdsyn_pipe_config_cmd)*src->cmdc))) return -1;
    dst->cmdc=dst->cmda=src->cmdc;
    memcpy(dst->cmdv,src->cmdv,sizeof(struct stdsyn_pipe_config_cmd)*src->cmdc);
    struct stdsyn_pipe_config_cmd *cmd=dst->cmdv;
    int i=dst->cmdc;
    for (;i-->0;cmd++) {
      if (!cmd->argc) continue;
      void *nv=malloc(cmd->argc);
      if (!nv) {
        dst->cmdc=cmd-dst->cmdv;
        stdsyn_pipe_config_cleanup(dst);
        return -1;
      }
      memcpy(nv,cmd->argv,cmd->argc);
      cmd->argv=nv;
    }
  } else {
    dst->cmdv=0;
    dst->cmda=0;
  }
  return 0;
}

/* Config add command.
 */
 
struct stdsyn_pipe_config_cmd *stdsyn_pipe_config_add_command(struct stdsyn_pipe_config *config) {
  if (config->cmdc>=config->cmda) {
    int na=config->cmda+4;
    if (na>INT_MAX/sizeof(struct stdsyn_pipe_config_cmd)) return 0;
    void *nv=realloc(config->cmdv,sizeof(struct stdsyn_pipe_config_cmd)*na);
    if (!nv) return 0;
    config->cmdv=nv;
    config->cmda=na;
  }
  struct stdsyn_pipe_config_cmd *cmd=config->cmdv+config->cmdc++;
  memset(cmd,0,sizeof(struct stdsyn_pipe_config_cmd));
  return cmd;
}

/* Pipe cleanup.
 */
 
static void stdsyn_pipe_cmd_cleanup(struct stdsyn_pipe_cmd *cmd) {
  stdsyn_node_del(cmd->node);
}

void stdsyn_pipe_cleanup(struct stdsyn_pipe *pipe) {
  if (pipe->cmdv) {
    while (pipe->cmdc-->0) stdsyn_pipe_cmd_cleanup(pipe->cmdv+pipe->cmdc);
    free(pipe->cmdv);
  }
  memset(pipe,0,sizeof(struct stdsyn_pipe));
}

/* Pipe init.
 */
 
int stdsyn_pipe_init(
  struct stdsyn_pipe *pipe,
  const struct stdsyn_pipe_config *config,
  struct bigpc_synth_driver *driver,
  uint8_t noteid,uint8_t velocity
) {
  if (!pipe||!config||!driver) return -1;
  memset(pipe,0,sizeof(struct stdsyn_pipe));
  pipe->defunct=-1; // by default, not defunctable
  if (config->cmdc) {
    if (!(pipe->cmdv=malloc(sizeof(struct stdsyn_pipe_cmd)*config->cmdc))) return -1;
    struct stdsyn_pipe_cmd *cmd=pipe->cmdv;
    const struct stdsyn_pipe_config_cmd *cfgcmd=config->cmdv;
    for (;pipe->cmdc<config->cmdc;cmd++,cfgcmd++,pipe->cmdc++) {
    
      if (cfgcmd->bufid>=STDSYN_BUFFER_COUNT) return -1;
      cmd->bufid=cfgcmd->bufid;
      if (!(cmd->node=stdsyn_node_new(driver,cfgcmd->type,1,cfgcmd->overwrite,noteid,velocity,cfgcmd->argv,cfgcmd->argc))) return -1;
      if (!cmd->node->update) return -1;
      
      if (cmd->node->release) pipe->defunct=0;
    }
  }
  return 0;
}

/* Pipe update.
 */

void stdsyn_pipe_update(struct stdsyn_pipe *pipe,int c) {
  int pendingc=0,defunctc=0;
  struct stdsyn_pipe_cmd *cmd=pipe->cmdv;
  int i=pipe->cmdc;
  for (;i-->0;cmd++) {
    cmd->node->update(stdsyn_node_get_buffer(cmd->node,cmd->bufid),c,cmd->node);
    if (cmd->node->release) {
      if (cmd->node->defunct) defunctc++;
      else pendingc++;
    }
  }
  if (pendingc) return;
  if (defunctc) pipe->defunct=1;
}

/* Release.
 */
 
void stdsyn_pipe_release(struct stdsyn_pipe *pipe,uint8_t velocity) {
  int released=0;
  struct stdsyn_pipe_cmd *cmd=pipe->cmdv;
  int i=pipe->cmdc;
  for (;i-->0;cmd++) {
    if (cmd->node->release) {
      released=1;
      cmd->node->release(cmd->node,velocity);
    }
  }
  if (!released) pipe->defunct=1;
}
