/* FluidSynth - A Software Synthesizer
 *
 * Copyright (C) 2003  Peter Hanappe and others.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *  
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 */

#include "fluid_ramsfont.h"
#include "fluid_sys.h"
#include "fluid_synth.h"	// for fluid_synth_add_sfont
 
/* thenumber of samples before the start and after the end */
#define SAMPLE_MARGIN 8

/* Prototypes */
int fluid_rampreset_add_sample(fluid_rampreset_t* preset, fluid_sample_t* sample, int lokey, int hikey);
int fluid_rampreset_izone_set_gen(fluid_rampreset_t* preset, fluid_sample_t* sample, int gen_type, float value);
int fluid_rampreset_izone_set_loop(fluid_rampreset_t* preset, fluid_sample_t* sample, int on, float loopstart, float loopend);
int fluid_rampreset_remove_izone(fluid_rampreset_t* preset, fluid_sample_t* sample);

/*
 * fluid_ramsfont_create_sfont
 */
fluid_sfont_t*
fluid_ramsfont_create_sfont()
{
	fluid_sfont_t* sfont;
	fluid_ramsfont_t* ramsfont;
	
	ramsfont = new_fluid_ramsfont();
	if (ramsfont == NULL) {
    return NULL;
  }
  
  sfont = FLUID_NEW(fluid_sfont_t);
  if (sfont == NULL) {
    FLUID_LOG(FLUID_ERR, "Out of memory");
    return NULL;
  }

  sfont->data = ramsfont;
  sfont->free = fluid_ramsfont_sfont_delete;
  sfont->get_name = fluid_ramsfont_sfont_get_name;
  sfont->get_preset = fluid_ramsfont_sfont_get_preset;
  sfont->iteration_start = fluid_ramsfont_sfont_iteration_start;
  sfont->iteration_next = fluid_ramsfont_sfont_iteration_next;

  return sfont;

}

/*
 * fluid_synth_add_sfont
 */
/* Could be in fluid_synth.c */
int fluid_synth_add_sfont(fluid_synth_t* synth, fluid_sfont_t* sfont)
{
	sfont->id = synth->sfont_id++;

	/* insert the sfont as the first one on the list */
	synth->sfont = fluid_list_prepend(synth->sfont, sfont);
	
	/* reset the presets for all channels */
	fluid_synth_program_reset(synth);
	
	return sfont->id;
}
 
/***************************************************************
 *
 *                           PUBLIC INTERFACE
 */

int fluid_ramsfont_sfont_delete(fluid_sfont_t* sfont)
{
	if (delete_fluid_ramsfont(sfont->data) != 0)
		return -1;
	FLUID_FREE(sfont);
	return 0;
}

char* fluid_ramsfont_sfont_get_name(fluid_sfont_t* sfont)
{
  return fluid_ramsfont_get_name((fluid_ramsfont_t*) sfont->data);
}

fluid_preset_t* fluid_ramsfont_sfont_get_preset(fluid_sfont_t* sfont, unsigned int bank, unsigned int prenum)
{
  fluid_preset_t* preset;
  fluid_rampreset_t* rampreset;

  rampreset = fluid_ramsfont_get_preset((fluid_ramsfont_t*) sfont->data, bank, prenum);

  if (rampreset == NULL) {
    return NULL;
  }

  preset = FLUID_NEW(fluid_preset_t);
  if (preset == NULL) {
    FLUID_LOG(FLUID_ERR, "Out of memory");
    return NULL;
  }

  preset->data = rampreset;
  preset->free = fluid_rampreset_preset_delete;
  preset->get_name = fluid_rampreset_preset_get_name;
  preset->get_banknum = fluid_rampreset_preset_get_banknum;
  preset->get_num = fluid_rampreset_preset_get_num;
  preset->noteon = fluid_rampreset_preset_noteon;
  preset->notify = NULL;

  return preset;
}

void fluid_ramsfont_sfont_iteration_start(fluid_sfont_t* sfont)
{
  fluid_ramsfont_iteration_start((fluid_ramsfont_t*) sfont->data);
}

int fluid_ramsfont_sfont_iteration_next(fluid_sfont_t* sfont, fluid_preset_t* preset)
{
  preset->free = fluid_rampreset_preset_delete;
  preset->get_name = fluid_rampreset_preset_get_name;
  preset->get_banknum = fluid_rampreset_preset_get_banknum;
  preset->get_num = fluid_rampreset_preset_get_num;
  preset->noteon = fluid_rampreset_preset_noteon;
  preset->notify = NULL;

  return fluid_ramsfont_iteration_next((fluid_ramsfont_t*) sfont->data, preset);
}

int fluid_rampreset_preset_delete(fluid_preset_t* preset)
{
  FLUID_FREE(preset);
  printf("TODO: free modulators\n"); 
  return 0;
}

char* fluid_rampreset_preset_get_name(fluid_preset_t* preset)
{
  return fluid_rampreset_get_name((fluid_rampreset_t*) preset->data);  
}

int fluid_rampreset_preset_get_banknum(fluid_preset_t* preset)
{
  return fluid_rampreset_get_banknum((fluid_rampreset_t*) preset->data);  
}

int fluid_rampreset_preset_get_num(fluid_preset_t* preset)
{
  return fluid_rampreset_get_num((fluid_rampreset_t*) preset->data);  
}

int fluid_rampreset_preset_noteon(fluid_preset_t* preset, fluid_synth_t* synth, int chan, int key, int vel)
{
  return fluid_rampreset_noteon((fluid_rampreset_t*) preset->data, synth, chan, key, vel);  
}



 
/***************************************************************
 *
 *                           SFONT
 */

/*
 * new_fluid_ramsfont
 */
fluid_ramsfont_t* new_fluid_ramsfont()
{
  fluid_ramsfont_t* sfont;

  sfont = FLUID_NEW(fluid_ramsfont_t);
  if (sfont == NULL) {
    FLUID_LOG(FLUID_ERR, "Out of memory");
    return NULL;
  }

  sfont->name[0] = 0;
  sfont->sample = NULL;
  sfont->preset = NULL;

  return sfont;
}

/*
 * delete_fluid_ramsfont
 */
int delete_fluid_ramsfont(fluid_ramsfont_t* sfont)
{
  fluid_list_t *list;
  fluid_rampreset_t* preset;

  /* Check that no samples are currently used */
  for (list = sfont->sample; list; list = fluid_list_next(list)) {
    fluid_sample_t* sam = (fluid_sample_t*) fluid_list_get(list);
    if (fluid_sample_refcount(sam) != 0) {
      return -1;
    }
  }

  for (list = sfont->sample; list; list = fluid_list_next(list)) {
  	/* in ram soundfonts, the samples hold their data : so we should free it ourselves */
  	fluid_sample_t* sam = (fluid_sample_t*)fluid_list_get(list);
    delete_fluid_ramsample(sam);
  }

  if (sfont->sample) {
    delete_fluid_list(sfont->sample);
  }

  preset = sfont->preset;
  while (preset != NULL) {
    sfont->preset = preset->next;
    delete_fluid_rampreset(preset);
    preset = sfont->preset;
  }
  
  FLUID_FREE(sfont);
  return FLUID_OK;
}

/*
 * fluid_ramsfont_get_name
 */
char* fluid_ramsfont_get_name(fluid_ramsfont_t* sfont)
{
  return sfont->name;
}

/*
 * fluid_ramsfont_set_name
 */
int
fluid_ramsfont_set_name(fluid_ramsfont_t* sfont, char * name)
{
  FLUID_STRCPY(sfont->name, name);
  return FLUID_OK;
}


/* fluid_ramsfont_add_preset
 *
 * Add a preset to the SoundFont
 */
int fluid_ramsfont_add_preset(fluid_ramsfont_t* sfont, fluid_rampreset_t* preset) 
{
  fluid_rampreset_t *cur, *prev;
  if (sfont->preset == NULL) {
    preset->next = NULL;
    sfont->preset = preset;
  } else {
    /* sort them as we go along. very basic sorting trick. */
    cur = sfont->preset;
    prev = NULL;
    while (cur != NULL) {
      if ((preset->bank < cur->bank) 
	  || ((preset->bank == cur->bank) && (preset->num < cur->num))) {
	if (prev == NULL) {
	  preset->next = cur;
	  sfont->preset = preset;
	} else {
	  preset->next = cur;
	  prev->next = preset;
	}
	return FLUID_OK;
      }
      prev = cur;
      cur = cur->next;
    }
    preset->next = NULL;
    prev->next = preset;
  }
  return FLUID_OK;
}

/*
 * fluid_ramsfont_add_ramsample
 */
int fluid_ramsfont_add_izone(fluid_ramsfont_t* sfont,
				unsigned int bank, unsigned int num, fluid_sample_t* sample,
				int lokey, int hikey)
{
	/*- find or create a preset
		- add it the sample using the fluid_rampreset_add_sample fucntion
		- add the sample to the list of samples
	*/
	int err;
	
	fluid_rampreset_t* preset = fluid_ramsfont_get_preset(sfont, bank, num);
	if (preset == NULL) {
		// Create it
		preset = new_fluid_rampreset(sfont);
		if (preset == NULL) {
			return FLUID_FAILED;
		}
		
		preset->bank = bank;
		preset->num = num;
		
		err = fluid_rampreset_add_sample(preset, sample, lokey, hikey);
		if (err != FLUID_OK) {
			return FLUID_FAILED;
		}
		
		// sort the preset
		fluid_ramsfont_add_preset(sfont, preset);
		
	} else {
	
		// just add it
		err = fluid_rampreset_add_sample(preset, sample, lokey, hikey);
		if (err != FLUID_OK) {
			return FLUID_FAILED;
		}	
	}
	
  sfont->sample = fluid_list_append(sfont->sample, sample);
  return FLUID_OK;
}

int fluid_ramsfont_remove_izone(fluid_ramsfont_t* sfont,
        unsigned int bank, unsigned int num, fluid_sample_t* sample) {
	fluid_rampreset_t* preset = fluid_ramsfont_get_preset(sfont, bank, num);
	if (preset == NULL) {
			return FLUID_FAILED;
	}
	return fluid_rampreset_remove_izone(preset, sample);
}


int fluid_ramsfont_izone_set_gen(fluid_ramsfont_t* sfont,
				unsigned int bank, unsigned int num, fluid_sample_t* sample,
				int gen_type, float value) {
				
	fluid_rampreset_t* preset = fluid_ramsfont_get_preset(sfont, bank, num);
	if (preset == NULL) {
			return FLUID_FAILED;
	}
	
	return fluid_rampreset_izone_set_gen(preset, sample, gen_type, value);
}

int fluid_ramsfont_izone_set_loop(fluid_ramsfont_t* sfont,
				unsigned int bank, unsigned int num, fluid_sample_t* sample,
				int on, float loopstart, float loopend) {
	fluid_rampreset_t* preset = fluid_ramsfont_get_preset(sfont, bank, num);
	if (preset == NULL) {
			return FLUID_FAILED;
	}
	
	return fluid_rampreset_izone_set_loop(preset, sample, on, loopstart, loopend);
}

/*
 * fluid_ramsfont_get_preset
 */
fluid_rampreset_t* fluid_ramsfont_get_preset(fluid_ramsfont_t* sfont, unsigned int bank, unsigned int num) 
{
  fluid_rampreset_t* preset = sfont->preset;
  while (preset != NULL) {
    if ((preset->bank == bank) && ((preset->num == num))) {
      return preset;
    }
    preset = preset->next;
  }
  return NULL;
}

/*
 * fluid_ramsfont_iteration_start
 */
void fluid_ramsfont_iteration_start(fluid_ramsfont_t* sfont)
{
  sfont->iter_cur = sfont->preset;
}

/*
 * fluid_ramsfont_iteration_next
 */
int fluid_ramsfont_iteration_next(fluid_ramsfont_t* sfont, fluid_preset_t* preset)
{
  if (sfont->iter_cur == NULL) {
    return 0;
  }

  preset->data = (void*) sfont->iter_cur;
  sfont->iter_cur = fluid_rampreset_next(sfont->iter_cur);
  return 1;
}

/***************************************************************
 *
 *                           PRESET
 */

/*
 * new_fluid_rampreset
 */
fluid_rampreset_t* 
new_fluid_rampreset(fluid_ramsfont_t* sfont)
{
  fluid_rampreset_t* preset = FLUID_NEW(fluid_rampreset_t);
  if (preset == NULL) {
    FLUID_LOG(FLUID_ERR, "Out of memory");     
    return NULL;
  }
  preset->next = NULL;
  preset->sfont = sfont;
  preset->name[0] = 0;
  preset->bank = 0;
  preset->num = 0;
  preset->global_zone = NULL;
  preset->zone = NULL;
  return preset;
}

/*
 * delete_fluid_rampreset
 */
int 
delete_fluid_rampreset(fluid_rampreset_t* preset)
{
  int err = FLUID_OK;
  fluid_preset_zone_t* zone;
  if (preset->global_zone != NULL) {
    if (delete_fluid_preset_zone(preset->global_zone) != FLUID_OK) {
      err = FLUID_FAILED;
    }
    preset->global_zone = NULL;
  }
  zone = preset->zone;
  while (zone != NULL) {
    preset->zone = zone->next;
    if (delete_fluid_preset_zone(zone) != FLUID_OK) {
      err = FLUID_FAILED;
    }
    zone = preset->zone;
  }
  FLUID_FREE(preset);
  return err;
}

int 
fluid_rampreset_get_banknum(fluid_rampreset_t* preset)
{
  return preset->bank;
}

int 
fluid_rampreset_get_num(fluid_rampreset_t* preset)
{
  return preset->num;
}

char* 
fluid_rampreset_get_name(fluid_rampreset_t* preset)
{
  return preset->name;
}

/*
 * fluid_rampreset_next
 */
fluid_rampreset_t* 
fluid_rampreset_next(fluid_rampreset_t* preset)
{
  return preset->next;
}


/*
 * fluid_rampreset_add_zone
 */
int 
fluid_rampreset_add_zone(fluid_rampreset_t* preset, fluid_preset_zone_t* zone)
{
  if (preset->zone == NULL) {
    zone->next = NULL;
    preset->zone = zone;
  } else {
    zone->next = preset->zone;
    preset->zone = zone;
  }
  return FLUID_OK;
}


/*
 * fluid_rampreset_add_sample
 */
int fluid_rampreset_add_sample(fluid_rampreset_t* preset, fluid_sample_t* sample, int lokey, int hikey)
{
	/* create a new instrument zone, with the given sample */
		
	/* one preset zone */
	if (preset->zone == NULL) {
		fluid_preset_zone_t* zone;
		zone = new_fluid_preset_zone("");
		if (zone == NULL) {
			return FLUID_FAILED;
		}
		
		/* its instrument */
		zone->inst = (fluid_inst_t*) new_fluid_inst();
    if (zone->inst == NULL) {
      delete_fluid_preset_zone(zone);
      return FLUID_FAILED;
    }
    
		fluid_rampreset_add_zone(preset, zone);
	}
	
	/* add an instrument zone for each sample */
	{
		fluid_inst_t* inst = fluid_preset_zone_get_inst(preset->zone);
		fluid_inst_zone_t* izone = new_fluid_inst_zone("");
		if (izone == NULL) {
			return FLUID_FAILED;
		}
	
		if (fluid_inst_add_zone(inst, izone) != FLUID_OK) {
			delete_fluid_inst_zone(izone);
			return FLUID_FAILED;
		}
		
		izone->sample = sample;
		izone->keylo = lokey;
		izone->keyhi = hikey;
		FLUID_STRCPY(preset->name, sample->name);
	}
	
	return FLUID_OK;
}

fluid_inst_zone_t* fluid_rampreset_izoneforsample(fluid_rampreset_t* preset, fluid_sample_t* sample)
{
	fluid_inst_t* inst;
	fluid_inst_zone_t* izone;
	
	if (preset->zone == NULL) return NULL;
	
	inst = fluid_preset_zone_get_inst(preset->zone);
	izone = inst->zone;
	while (izone) {
		if (izone->sample == sample)
			return izone;
		izone = izone->next;
	}
	return NULL;
}

int fluid_rampreset_izone_set_loop(fluid_rampreset_t* preset, fluid_sample_t* sample,
       int on, float loopstart, float loopend) {
	fluid_inst_zone_t* izone = fluid_rampreset_izoneforsample(preset, sample);
	short coarse, fine;
	
	if (izone == NULL)
			return FLUID_FAILED;
	
	if (!on) {
		izone->gen[GEN_SAMPLEMODE].flags = GEN_SET;
		izone->gen[GEN_SAMPLEMODE].val = FLUID_UNLOOPED;
		return FLUID_OK;
}
	
	izone->gen[GEN_SAMPLEMODE].flags = GEN_SET;
	izone->gen[GEN_SAMPLEMODE].val = FLUID_LOOP_DURING_RELEASE;
	
	/* loopstart */
	if (loopstart > 32767. || loopstart < -32767.) {
		coarse = (short)(loopend/32767.);
		fine = (short)(loopend - (float)(coarse)*32767.);
	} else {
		coarse = 0;
		fine = (short)loopstart;
	}
	izone->gen[GEN_STARTLOOPADDROFS].flags = GEN_SET;
	izone->gen[GEN_STARTLOOPADDROFS].val = fine;
	if (coarse) {
		izone->gen[GEN_STARTLOOPADDRCOARSEOFS].flags = GEN_SET;
		izone->gen[GEN_STARTLOOPADDRCOARSEOFS].val = coarse;
	} else {
		izone->gen[GEN_STARTLOOPADDRCOARSEOFS].flags = GEN_UNUSED;
	}

	/* loopend */
	if (loopend > 32767. || loopend < -32767.) {
		coarse = (short)(loopend/32767.);
		fine = (short)(loopend - (float)(coarse)*32767.);
	} else {
		coarse = 0;
		fine = (short)loopend;
	}
	izone->gen[GEN_ENDLOOPADDROFS].flags = GEN_SET;
	izone->gen[GEN_ENDLOOPADDROFS].val = fine;
	if (coarse) {
		izone->gen[GEN_ENDLOOPADDRCOARSEOFS].flags = GEN_SET;
		izone->gen[GEN_ENDLOOPADDRCOARSEOFS].val = coarse;
	} else {
		izone->gen[GEN_ENDLOOPADDRCOARSEOFS].flags = GEN_UNUSED;
	}
	
	/* If the loop points are the whole samples, we are supposed to
	   copy the frames around in the margins (the start to the end margin and
	   the end to the start margin), but it works fine without this. Maybe some time
	   it will be needed */
	
	return FLUID_OK;
}

int fluid_rampreset_izone_set_gen(fluid_rampreset_t* preset, fluid_sample_t* sample,
       int gen_type, float value) {
	fluid_inst_zone_t* izone = fluid_rampreset_izoneforsample(preset, sample);
	if (izone == NULL)
			return FLUID_FAILED;
		
	izone->gen[gen_type].flags = GEN_SET;
	izone->gen[gen_type].val = value;
	return FLUID_OK;
}

int fluid_rampreset_remove_izone(fluid_rampreset_t* preset, fluid_sample_t* sample) {
	fluid_inst_t* inst;
	fluid_inst_zone_t* izone, * prev;;
	
	if (preset->zone == NULL) return FLUID_FAILED;
	inst = fluid_preset_zone_get_inst(preset->zone);
	izone = inst->zone;
	prev = NULL;
	while (izone) {
		if (izone->sample == sample) {
			if (prev == NULL) {
				inst->zone = izone->next;
			} else {
				prev->next = izone->next;
			}
			izone->next = NULL;
			delete_fluid_inst_zone(izone);
			return FLUID_OK;
		}
		prev = izone;
		izone = izone->next;
	}
	return FLUID_FAILED;
}

/* these conversions taken from SF2.0 specs
#ifdef WIN32
#define log2(x) (log(x)/log(2.0))
#endif
		if (loop) {
			izone->gen[Gen_SampleModes].flags = GEN_SET;
			izone->gen[Gen_SampleModes].val = FLUID_LOOP_DURING_RELEASE;
		}
		if (attack != 0.0) {
			izone->gen[Gen_VolEnvAttack].flags = GEN_SET;
			izone->gen[Gen_VolEnvAttack].val = 1200.0*log2(attack/1000.0);
		}
		if (decay != 0.0) {
			izone->gen[Gen_VolEnvDecay].flags = GEN_SET;
			izone->gen[Gen_VolEnvDecay].val = 1200.0*log2(decay/1000.0);
		}		
		if (sustain != 0.0) {
			int sustainInt = sustain;
			izone->gen[Gen_VolEnvSustain].flags = GEN_SET;
			izone->gen[Gen_VolEnvSustain].val = sustainInt*10;
		}
		if (release != 0.0) {
			izone->gen[Gen_VolEnvRelease].flags = GEN_SET;
			izone->gen[Gen_VolEnvRelease].val = 1200.0*log2(release/1000.0);
		}
*/			


/*
 * fluid_rampreset_noteon
 */
int 
fluid_rampreset_noteon(fluid_rampreset_t* preset, fluid_synth_t* synth, int chan, int key, int vel) 
{
  fluid_preset_zone_t *preset_zone;
  fluid_inst_t* inst;
  fluid_inst_zone_t *inst_zone, *global_inst_zone, *z;
  fluid_sample_t* sample;
  fluid_voice_t* voice;
  fluid_mod_t * mod;
  fluid_mod_t * mod_list[FLUID_NUM_MOD]; /* list for 'sorting' preset modulators */
  int mod_list_count;
  int i;
  
  /* run thru all the zones of this preset */
  preset_zone = preset->zone;
  while (preset_zone != NULL) {

    /* check if the note falls into the key and velocity range of this
       preset */
    if (fluid_preset_zone_inside_range(preset_zone, key, vel)) {
      
      inst = fluid_preset_zone_get_inst(preset_zone);
      global_inst_zone = fluid_inst_get_global_zone(inst);
      
      /* run thru all the zones of this instrument */
      inst_zone = fluid_inst_get_zone(inst);
      while (inst_zone != NULL) {
	
	/* make sure this instrument zone has a valid sample */
	sample = fluid_inst_zone_get_sample(inst_zone);
	if (fluid_sample_in_rom(sample) || (sample == NULL)) {
	  inst_zone = fluid_inst_zone_next(inst_zone);
	  continue;
	}
	
	/* check if the note falls into the key and velocity range of this
	   instrument */

	if (fluid_inst_zone_inside_range(inst_zone, key, vel) && (sample != NULL)) {
	  
	  /* this is a good zone. allocate a new synthesis process and
             initialize it */

	  voice = fluid_synth_alloc_voice(synth, sample, chan, key, vel);
	  if (voice == NULL) {
	    return FLUID_FAILED; 
	  }
	  
	  
	  z = inst_zone;
	  
	  /* Instrument level, generators */
	  
	  for (i = 0; i < GEN_LAST; i++) {

	    /* SF 2.01 section 9.4 'bullet' 4: 
	     *
	     * A generator in a local instrument zone supersedes a
	     * global instrument zone generator.  Both cases supersede
	     * the default generator -> voice_gen_set */

	    if (inst_zone->gen[i].flags){
	      fluid_voice_gen_set(voice, i, inst_zone->gen[i].val);

	    } else if (global_inst_zone != NULL && global_inst_zone->gen[i].flags){
	      fluid_voice_gen_set(voice, i, global_inst_zone->gen[i].val);		  

	    } else {
	      /* The generator has not been defined in this instrument.
	       * Do nothing, leave it at the default.
	       */
	    };

	  }; /* for all generators */
	  
	  /* global instrument zone, modulators: Put them all into a
	   * list. */

	  mod_list_count = 0;

	  if (global_inst_zone){
	    mod = global_inst_zone->mod;
	    while (mod){
	      mod_list[mod_list_count++] = mod;
	      mod = mod->next;
	    };
	  };
	  
	  /* local instrument zone, modulators.
	   * Replace modulators with the same definition in the list:
	   * SF 2.01 page 69, 'bullet' 8
	   */
	  mod = inst_zone->mod;

	  while (mod){

	    /* 'Identical' modulators will be deleted by setting their
	     *  list entry to NULL.  The list length is known, NULL
	     *  entries will be ignored later.  SF2.01 section 9.5.1
	     *  page 69, 'bullet' 3 defines 'identical'.  */

	    for (i = 0; i < mod_list_count; i++){
	      if (fluid_mod_test_identity(mod,mod_list[i])){
		mod_list[i] = NULL;
	      };
	    };
	      
	    /* Finally add the new modulator to to the list. */
	    mod_list[mod_list_count++] = mod;
	    mod = mod->next;
	  };

	  /* Add instrument modulators (global / local) to the voice. */
	  for (i = 0; i < mod_list_count; i++){

	    mod = mod_list[i];

	    if (mod != NULL){ /* disabled modulators CANNOT be skipped. */

	      /* Instrument modulators -supersede- existing (default)
	       * modulators.  SF 2.01 page 69, 'bullet' 6 */
	      fluid_voice_add_mod(voice, mod, FLUID_VOICE_OVERWRITE);
	    };
	  };

	  /* Preset level, generators */

	  for (i = 0; i < GEN_LAST; i++) {

	    /* SF 2.01 section 8.5 page 58: If some generators are
	     * encountered at preset level, they should be ignored */
	    if ((i != GEN_STARTADDROFS) 
		&& (i != GEN_ENDADDROFS) 
		&& (i != GEN_STARTLOOPADDROFS) 
		&& (i != GEN_ENDLOOPADDROFS) 
		&& (i != GEN_STARTADDRCOARSEOFS) 
		&& (i != GEN_ENDADDRCOARSEOFS) 
		&& (i != GEN_STARTLOOPADDRCOARSEOFS)
		&& (i != GEN_KEYNUM)
		&& (i != GEN_VELOCITY)
		&& (i != GEN_ENDLOOPADDRCOARSEOFS)
		&& (i != GEN_SAMPLEMODE)
		&& (i != GEN_EXCLUSIVECLASS)
		&& (i != GEN_OVERRIDEROOTKEY)) {

	      /* SF 2.01 section 9.4 'bullet' 9: A generator in a
	       * local preset zone supersedes a global preset zone
	       * generator.  The effect is -added- to the destination
	       * summing node -> voice_gen_incr */

	      if (preset_zone->gen[i].flags){
		fluid_voice_gen_incr(voice, i, preset_zone->gen[i].val);
	      } else {
		/* The generator has not been defined in this preset
		 * Do nothing, leave it unchanged.
		 */
	      };
	    }; /* if available at preset level */
	  }; /* for all generators */
	  
	  
	  /* Global preset zone, modulators: put them all into a
	   * list. */
	  mod_list_count = 0;

	  /* Process the modulators of the local preset zone.  Kick
	   * out all identical modulators from the global preset zone
	   * (SF 2.01 page 69, second-last bullet) */

	  mod = preset_zone->mod;
	  while (mod){
	    for (i = 0; i < mod_list_count; i++){
	      if (fluid_mod_test_identity(mod,mod_list[i])){
		mod_list[i] = NULL;
	      };
	    };
	      
	    /* Finally add the new modulator to the list. */
	    mod_list[mod_list_count++] = mod;
	    mod = mod->next;
	  };
	  
	  /* Add preset modulators (global / local) to the voice. */
	  for (i = 0; i < mod_list_count; i++){
	    mod = mod_list[i];
	    if ((mod != NULL) && (mod->amount != 0)) { /* disabled modulators can be skipped. */

	      /* Preset modulators -add- to existing instrument /
	       * default modulators.  SF2.01 page 70 first bullet on
	       * page */
	      fluid_voice_add_mod(voice, mod, FLUID_VOICE_ADD);
	    };
	  };	  

	  /* add the synthesis process to the synthesis loop. */
	  fluid_synth_start_voice(synth, voice);

	  /* Store the ID of the first voice that was created by this noteon event.
	   * Exclusive class may only terminate older voices.
	   * That avoids killing voices, which have just been created.
	   * (a noteon event can create several voice processes with the same exclusive
	   * class - for example when using stereo samples)
	   */
	}

	inst_zone = fluid_inst_zone_next(inst_zone);
      }
    }
    preset_zone = fluid_preset_zone_next(preset_zone);
  }

  return FLUID_OK;
}



/***************************************************************
 *
 *                           SAMPLE
 */



/*
 * fluid_sample_set_name
 */
int
fluid_sample_set_name(fluid_sample_t* sample, char * name)
{
  FLUID_STRCPY(sample->name, name);
  return FLUID_OK;
}

/*
 * fluid_sample_set_sound_data
 */
int
fluid_sample_set_sound_data(fluid_sample_t* sample, short *data, unsigned int nbframes, short copy_data, int rootkey)
{
	/* 16 bit mono 44.1KHz data in */
	/* in all cases, the sample has ownership of the data : it will release it in the end */
	int firstReal, lastreal;
	
	/* in case we already have some data */
  if (sample->data != NULL) {
  	FLUID_FREE(sample->data);
  }
	
	if (copy_data) {
	  sample->data = FLUID_MALLOC(nbframes*2 + 4*SAMPLE_MARGIN);
	  if (sample->data == NULL) {
	    FLUID_LOG(FLUID_ERR, "Out of memory");
	    return FLUID_FAILED;
	  }
	  FLUID_MEMSET(sample->data, 0, nbframes*2 + 4*SAMPLE_MARGIN);
	  FLUID_MEMCPY((sample->data) + 2*SAMPLE_MARGIN, data, nbframes*2);
  } else {
  	sample->data = data;
  }
  
  /* pointers */
  /* all from the start of data */
  firstReal = SAMPLE_MARGIN;
  lastreal = SAMPLE_MARGIN + nbframes;
  
  /* one frame before first, to leave room for loopstart */
  sample->start = firstReal - 1;
  /* 2 frames after last, to leave room for loopend */
  sample->end = lastreal + 2;
  /* only used as markers for the LOOP generators : set them on the first real frame */
  sample->loopstart = firstReal;
  sample->loopend = lastreal + 1;
  
  sample->samplerate = 44100;
  sample->origpitch = rootkey;
  sample->pitchadj = 0;
  sample->sampletype = FLUID_SAMPLETYPE_MONO;
  sample->valid = 1;

  return FLUID_OK;
}

/*
 * new_fluid_ramsample
 */
fluid_sample_t* 
new_fluid_ramsample()
{
	/* same as new_fluid_sample. Only here so that it is exported */
  fluid_sample_t* sample = NULL;

  sample = FLUID_NEW(fluid_sample_t);
  if (sample == NULL) {
    FLUID_LOG(FLUID_ERR, "Out of memory");     
    return NULL;
  }

  memset(sample, 0, sizeof(fluid_sample_t));

  return sample;
}

/*
 * delete_fluid_ramsample
 */
int
delete_fluid_ramsample(fluid_sample_t* sample)
{
	/* same as delete_fluid_sample, plus frees the data */
  if (sample->data != NULL) {
  	FLUID_FREE(sample->data);
  }
  sample->data = NULL;
  FLUID_FREE(sample);
  return FLUID_OK;
}