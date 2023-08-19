#include "inmgr_internal.h"
#include "app/fmn_platform.h" /* for FMN_INPUT_* */
#include "opt/bigpc/bigpc_internal.h" /* for BIGPC_ACTIONID_* */

/* Map device with a rule set.
 */
 
int inmgr_map_device(struct inmgr *inmgr,const struct inmgr_device *device,const struct inmgr_rules *rules) {
  if (!device||!rules) return -1;
  const struct inmgr_rules_button *button=rules->buttonv;
  int i=rules->buttonc;
  for (;i-->0;button++) {
    const struct inmgr_cap *cap=inmgr_device_get_capability(device,button->srcbtnid);
    
    if (!cap) {
      if (button->srcpart==INMGR_SRCPART_BUTTON_ON) {
        if (inmgr_mapv_add(inmgr,device->devid,button->srcbtnid,0,1,INT_MAX,button->dsttype,0,button->dstbtnid)<0) return -1;
      } else {
        // can't map rules other than BUTTON_ON for caps that don't exist. skip it.
      }
      
    } else switch (button->srcpart) {
    
      case INMGR_SRCPART_BUTTON_ON: {
          if (inmgr_mapv_add(inmgr,device->devid,button->srcbtnid,cap->lo,cap->lo+1,INT_MAX,button->dsttype,0,button->dstbtnid)<0) return -1;
        } break;
        
      case INMGR_SRCPART_AXIS_LOW: {
          int mid=(cap->lo+cap->hi)>>1;
          int midlo=(cap->lo+mid)>>1;
          if (midlo>=mid) midlo=mid-1;
          if (inmgr_mapv_add(inmgr,device->devid,button->srcbtnid,mid,INT_MIN,midlo,button->dsttype,0,button->dstbtnid)<0) return -1;
        } break;
        
      case INMGR_SRCPART_AXIS_HIGH: {
          int mid=(cap->lo+cap->hi)>>1;
          int midhi=(cap->hi+mid)>>1;
          if (midhi<=mid) midhi=mid+1;
          if (inmgr_mapv_add(inmgr,device->devid,button->srcbtnid,mid,midhi,INT_MAX,button->dsttype,0,button->dstbtnid)<0) return -1;
        } break;
        
      case INMGR_SRCPART_HAT_N: {
          if (inmgr_mapv_add(inmgr,device->devid,button->srcbtnid,cap->lo-1,cap->lo,cap->lo-1,button->dsttype,0,button->dstbtnid)<0) return -1;
        } break;
        
      case INMGR_SRCPART_HAT_E: {
          if (inmgr_mapv_add(inmgr,device->devid,button->srcbtnid,cap->lo-1,cap->lo+1,cap->lo+3,button->dsttype,0,button->dstbtnid)<0) return -1;
        } break;
        
      case INMGR_SRCPART_HAT_S: {
          if (inmgr_mapv_add(inmgr,device->devid,button->srcbtnid,cap->lo-1,cap->lo+3,cap->lo+5,button->dsttype,0,button->dstbtnid)<0) return -1;
        } break;
        
      case INMGR_SRCPART_HAT_W: {
          if (inmgr_mapv_add(inmgr,device->devid,button->srcbtnid,cap->lo-1,cap->lo+5,cap->lo+7,button->dsttype,0,button->dstbtnid)<0) return -1;
        } break;
        
    }
  }
  return 0;
}

/* Guess mappings from a device, by its declared capabilities.
 * We strongly prefer to map every button to something.
 * Because it's not known whether a declared button really exists physically.
 * Hats become the dpad, signed axes alternate horz/vert, and 2-state buttons alternate use/choose.
 * Unsigned axes are treated as 2-state buttons.
 * We'll ignore anything that doesn't fit one of these patterns, though that is pretty rare.
 */
 
int inmgr_guess_capability_role(const struct inmgr_cap *cap) {

  // If the range is smaller than 2, there's definitely nothing we can do with it.
  if (cap->hi<cap->lo+1) return 0;

  // Range of (0..1) or (0..2) is definitely a 2-state button.
  if ((cap->lo==0)&&((cap->hi==1)||(cap->hi==2))) return INMGR_CAP_ROLE_BUTTON;
  
  // On the 7 (keyboard) or 9 (generic button) page, it's a button.
  if ((cap->usage&0xffff0000)==0x00070000) return INMGR_CAP_ROLE_BUTTON;
  if ((cap->usage&0xffff0000)==0x00090000) return INMGR_CAP_ROLE_BUTTON;
  
  // (-1..1) is a very polite way for dpad axes to report; I wish they all did it like that.
  if ((cap->lo==-1)&&(cap->hi==1)) return INMGR_CAP_ROLE_AXIS;
  
  // A range of 8 values is probably a hat.
  // I've seen these anchored at both zero and one, and we'll tolerate any anchor value.
  // A wee quirk of ours: We expect at least one OOB value on each end of the range. So reject if it's at INT_MIN or INT_MAX.
  // (Why the hell would a device report like that? Well, why the hell is it using hats...)
  if (cap->hi-cap->lo==7) {
    if ((cap->lo==INT_MIN)||(cap->hi==INT_MAX)) return 0;
    return INMGR_CAP_ROLE_HAT;
  }
  
  // Range of at least three, it's an axis.
  // If both the low value and resting value are zero, assume it's one-way, like an analogue trigger button.
  // ...unless it claims a known HID axis that we should assume is three-way.
  // eg the 8bitdo Pro 2, a beautiful machine with fantastically poor reporting choices, its sticks are all 0..255.
  if (cap->hi-cap->lo>=2) {
    switch (cap->usage) {
      case 0x00010030:
      case 0x00010031:
      case 0x00010033:
      case 0x00010034:
      case 0x00010040:
      case 0x00010041:
      case 0x00010043:
      case 0x00010044:
        return INMGR_CAP_ROLE_AXIS;
    }
    if ((cap->lo==0)&&(cap->rest==0)) return INMGR_CAP_ROLE_BUTTON;
    return INMGR_CAP_ROLE_AXIS;
  }

  // OK I'm stumped.
  return 0;
}

static void inmgr_map_axis(struct inmgr *inmgr,int devid,const struct inmgr_cap *cap,uint16_t btnidlo,uint16_t btnidhi) {
  int mid=(cap->lo+cap->hi)>>1;
  int midlo=(cap->lo+mid)>>1;
  int midhi=(cap->hi+mid)>>1;
  if (midlo>=mid) midlo=mid-1;
  if (midhi<=mid) midhi=mid+1;
  inmgr_mapv_add(inmgr,devid,cap->btnid,mid,INT_MIN,midlo,INMGR_DSTTYPE_BUTTON,0,btnidlo);
  inmgr_mapv_add(inmgr,devid,cap->btnid,mid,midhi,INT_MAX,INMGR_DSTTYPE_BUTTON,0,btnidhi);
}
 
static int inmgr_map_device_per_caps(struct inmgr *inmgr,const struct inmgr_device *device) {
  //fprintf(stderr,"%s devid=%d %04x:%04x '%.*s'\n",__func__,device->devid,device->vid,device->pid,device->namec,device->name);
  //#define TATTLE(comment) fprintf(stderr,"  %s btnid=0x%08x usage=0x%08x range=%d..%d rest=%d\n",comment,cap->btnid,cap->usage,cap->lo,cap->hi,cap->rest);
  #define TATTLE(comment)
  int btnc=0,axisc=0,hatc=0,altaxisc=0;
  const struct inmgr_cap *cap=device->capv;
  int i=device->capc;
  for (;i-->0;cap++) {
    switch (cap->role) {
    
      case INMGR_CAP_ROLE_IGNORE: {
          TATTLE("IGNORE")
        } break;
      
      case INMGR_CAP_ROLE_BUTTON: {
          TATTLE("BUTTON")
          // A few well-defined usages can map to the dpad.
          switch (cap->usage) {
            case 0x0001008a: inmgr_mapv_add_button(device->devid,cap->btnid,RIGHT); altaxisc++; break; // System Menu Right
            case 0x0001008b: inmgr_mapv_add_button(device->devid,cap->btnid,LEFT); altaxisc++; break; // System Menu Left
            case 0x0001008c: inmgr_mapv_add_button(device->devid,cap->btnid,UP); altaxisc++; break; // System Menu Up
            case 0x0001008d: inmgr_mapv_add_button(device->devid,cap->btnid,DOWN); altaxisc++; break; // System Menu Down
            case 0x00010090: inmgr_mapv_add_button(device->devid,cap->btnid,UP); altaxisc++; break; // D-pad Up
            case 0x00010091: inmgr_mapv_add_button(device->devid,cap->btnid,DOWN); altaxisc++; break; // D-pad Down
            case 0x00010092: inmgr_mapv_add_button(device->devid,cap->btnid,RIGHT); altaxisc++; break; // D-pad Right
            case 0x00010093: inmgr_mapv_add_button(device->devid,cap->btnid,LEFT); altaxisc++; break; // D-pad Left
            default: { // most buttons will alternate USE and MENU
                if (btnc&1) inmgr_mapv_add_button(device->devid,cap->btnid,MENU);
                else inmgr_mapv_add_button(device->devid,cap->btnid,USE);
                btnc++;
              }
          }
        } break;
      
      case INMGR_CAP_ROLE_AXIS: {
          TATTLE("AXIS")
          // If it has an explicit usage, map that way, but toggle the low bit of axisc only if this was the axis we were expecting.
          // (trying to keep sensible behavior when there are both explicit and implicit rules).
          switch (cap->usage) {
            case 0x00010030: inmgr_map_axis(inmgr,device->devid,cap,FMN_INPUT_LEFT,FMN_INPUT_RIGHT); if (axisc&1) axisc+=2; else axisc++; break; // X
            case 0x00010031: inmgr_map_axis(inmgr,device->devid,cap,FMN_INPUT_UP,FMN_INPUT_DOWN); if (axisc&1) axisc++; else axisc+=2; break; // Y
            case 0x00010033: inmgr_map_axis(inmgr,device->devid,cap,FMN_INPUT_LEFT,FMN_INPUT_RIGHT); if (axisc&1) axisc+=2; else axisc++; break; // Rx
            case 0x00010034: inmgr_map_axis(inmgr,device->devid,cap,FMN_INPUT_UP,FMN_INPUT_DOWN); if (axisc&1) axisc++; else axisc+=2; break; // Ry
            case 0x00010040: inmgr_map_axis(inmgr,device->devid,cap,FMN_INPUT_LEFT,FMN_INPUT_RIGHT); if (axisc&1) axisc+=2; else axisc++; break; // Vx
            case 0x00010041: inmgr_map_axis(inmgr,device->devid,cap,FMN_INPUT_UP,FMN_INPUT_DOWN); if (axisc&1) axisc++; else axisc+=2; break; // Vy
            case 0x00010043: inmgr_map_axis(inmgr,device->devid,cap,FMN_INPUT_LEFT,FMN_INPUT_RIGHT); if (axisc&1) axisc+=2; else axisc++; break; // Vbrx
            case 0x00010044: inmgr_map_axis(inmgr,device->devid,cap,FMN_INPUT_UP,FMN_INPUT_DOWN); if (axisc&1) axisc++; else axisc+=2; break; // Vbry
            default: {
                if (axisc&1) inmgr_map_axis(inmgr,device->devid,cap,FMN_INPUT_UP,FMN_INPUT_DOWN);
                else inmgr_map_axis(inmgr,device->devid,cap,FMN_INPUT_LEFT,FMN_INPUT_RIGHT);
                axisc++;
              }
          }
        } break;
      
      case INMGR_CAP_ROLE_HAT: {
          TATTLE("HAT")
          // Hats are a pain in the ass. If there's a petition somewhere to efface these abominations from the face of the earth, please lemme know, I'll sign it twice.
          // The UP side is weird because its values are discontiguous (0,1,7). There's special handling for this case at inmgr_event().
          inmgr_mapv_add(inmgr,device->devid,cap->btnid,cap->hi+1,cap->lo,cap->lo-1,INMGR_DSTTYPE_BUTTON,0,FMN_INPUT_UP);
          // The other three sides are a bit more tractable.
          inmgr_mapv_add(inmgr,device->devid,cap->btnid,cap->hi+1,cap->lo+1,cap->lo+3,INMGR_DSTTYPE_BUTTON,0,FMN_INPUT_RIGHT);
          inmgr_mapv_add(inmgr,device->devid,cap->btnid,cap->hi+1,cap->lo+3,cap->lo+5,INMGR_DSTTYPE_BUTTON,0,FMN_INPUT_DOWN);
          inmgr_mapv_add(inmgr,device->devid,cap->btnid,cap->hi+1,cap->lo+5,cap->lo+7,INMGR_DSTTYPE_BUTTON,0,FMN_INPUT_LEFT);
          hatc++;
        } break;
      
    }
  }
  #undef TATTLE
  //fprintf(stderr,"...done mapping. btnc=%d axisc=%d hatc=%d altaxisc=%d\n",btnc,axisc,hatc,altaxisc);
  // If we didn't get all the required things, remove whatever we did add.
  // I'm not going to split hairs over cases with like, a single axis and 2 altaxis. That might be valid, but it's dubious and unlikely.
  if (
    (btnc<2)||
    (!hatc&&(axisc<2)&&(altaxisc<4))
  ) {
    //fprintf(stderr,"...mapping inadequate. Rejecting.\n");
    inmgr_mapv_remove_devid(inmgr,device->devid); // Could trigger events but shouldn't, since we added with straight zero dstvalues.
    return 0;
  }
  return 1;
}

/* Map what appears to be the system keyboard with defaults.
 */
 
static int inmgr_map_device_keyboard(struct inmgr *inmgr,const struct inmgr_device *device) {
  //fprintf(stderr,"%s devid=%d %04x:%04x '%.*s'\n",__func__,device->devid,device->vid,device->pid,device->namec,device->name);
  
  // Typical: WASD + Space + Enter
  inmgr_mapv_add_button(device->devid,0x00070004,LEFT);
  inmgr_mapv_add_button(device->devid,0x00070007,RIGHT);
  inmgr_mapv_add_button(device->devid,0x0007001a,UP);
  inmgr_mapv_add_button(device->devid,0x00070016,DOWN);
  inmgr_mapv_add_button(device->devid,0x0007002c,USE);
  inmgr_mapv_add_button(device->devid,0x00070028,MENU);
  
  // Also typical: Arrows + Z + X
  inmgr_mapv_add_button(device->devid,0x00070050,LEFT);
  inmgr_mapv_add_button(device->devid,0x0007004f,RIGHT);
  inmgr_mapv_add_button(device->devid,0x00070052,UP);
  inmgr_mapv_add_button(device->devid,0x00070051,DOWN);
  inmgr_mapv_add_button(device->devid,0x0007001d,USE);
  inmgr_mapv_add_button(device->devid,0x0007001b,MENU);
  
  // Numeric keypad 4,5,6,8,2 + 0=use + enter=choose
  inmgr_mapv_add_button(device->devid,0x0007005c,LEFT);
  inmgr_mapv_add_button(device->devid,0x0007005e,RIGHT);
  inmgr_mapv_add_button(device->devid,0x00070060,UP);
  inmgr_mapv_add_button(device->devid,0x0007005d,DOWN);
  inmgr_mapv_add_button(device->devid,0x0007005a,DOWN);
  inmgr_mapv_add_button(device->devid,0x00070062,USE);
  inmgr_mapv_add_button(device->devid,0x00070058,MENU);
  
  // Escape,F12 = Quit,Fullscreen
  inmgr_mapv_add_action(device->devid,0x00070029,quit);
  inmgr_mapv_add_action(device->devid,0x00070045,fullscreen);
  
  return 1;
}

/* Map device with inferred rules.
 * Return >0 if mapped satisfactorily.
 */
 
int inmgr_map_device_ruleless(struct inmgr *inmgr,const struct inmgr_device *device) {
  if (!device->capc&&inmgr_pattern_match("*keyboard*",-1,device->name,device->namec)) {
    return inmgr_map_device_keyboard(inmgr,device);
  }
  return inmgr_map_device_per_caps(inmgr,device);
}
