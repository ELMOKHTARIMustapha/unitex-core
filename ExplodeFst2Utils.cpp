 /*
  * Unitex
  *
  * Copyright (C) 2001-2008 Universit� Paris-Est Marne-la-Vall�e <unitex@univ-mlv.fr>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Lesser General Public
  * License as published by the Free Software Foundation; either
  * version 2.1 of the License, or (at your option) any later version.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Lesser General Public License for more details.
  * 
  * You should have received a copy of the GNU Lesser General Public
  * License along with this library; if not, write to the Free Software
  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
  *
  */

#include "autalmot_old.h"
#include "String_hash.h"
#include "fst2autalmot.h"
#include "utils.h"
#include "DELA.h"
#include "Transitions.h"


static unichar PONCTAB[] = {
  '"', '\'',
  '+', '-', '*', '\\', '=',
  '.', ',', ':', ';', '!', '?',
  '(', ')', '[', ']', '<', '>',
  '%', '#', '@', '/', '$', '&',
  '|', '_', 0
};


#if 0

void determine_sortesymbole(tSymbole * symb) {

  if (symb->sorteSymbole != INDETERMINE) { return; }


  if (complet(symb->gramm)) {

    switch (*symb->canonique) {

    case 0:
      symb->sorteSymbole = CODE_POUET;
      break;

    case '!':
      symb->sorteSymbole = NEGATIF;
      break;

    default:
      symb->sorteSymbole = ATOME;
    }

  } else {
    
    if (*symb->canonique) { die("bad symbol: canonical form (%S) with incomplete code (%S)\n", symb->canonique, symb->gramm); }

    symb->sorteSymbole = INCOMPLET;
  }
}


void symbole_strip(tSymbole * symb) {

  unichar * p, * q;

  for (p = symb->gramm; *p != '+'; p++) { if (*p == 0) return; }

  for (q = p + 1; (*q != 0) && (*q != ':'); q++);

  while ((*(p++) = *(q++)));
}

#endif


static int compte_flex(unichar * gramm) {

  int nb = 0;

  while (*gramm) {
    if (*gramm == ':') { nb++; }
    gramm++;
  }

  return nb;
}



// assume symb->sorteSymbole == ATOM

int symbole_developp(tAlphMot * alphabet, tSymbole * symb) {

  int nbflex = compte_flex(symb->gramm);

  if (nbflex < 2) {
    add_symbol(alphabet, symb);
    return 1;
  }

  int i, k, n;
  unichar common[maxGramm];
  unichar * p;

  for (i = 0; symb->gramm[i] != ':'; i++) { common[i] = symb->gramm[i]; }
  common[i] = 0;

  tSymbole * s = tSymbole_new();

  symbole_copy(s, symb);

  for (p = symb->gramm + i, n = 0; n < nbflex; n++) {

    unichar flex[maxGramm];
    k = 0; p++;

    while (*p && *p !=  ':') { flex[k++] = *(p++); }
    flex[k] = 0;

    u_sprintf(s->gramm, "%S:%S", common, flex);

    add_symbol(alphabet, s);
  }

  symbole_delete(s);

  return nbflex;
}


/*
static int check_dic_entry(const unichar * label) {

  // {__,__.__} 

  if (*label != '{') { error("label %S isn't a good DELA entry\n", label); return -1; }

  const unichar * p = label + 1;

  while (*p !=  ',') { // look for ','
    if (*p == 0) { error("malformed DELA entry: '%S' (',' is missing).\n", label); return -1; }
    p++;
  }
    
  while (*p != '.') {
    if (*p == 0) { error("malformed DELA entry: '%S' ('.' is missing).\n", label); return -1; }
    p++;
  }

  while (*p != '}') {
    if (*p == 0) { error("malformed DELA entry: '%S' ('}' is missing).\n", label); return -1; }
    p++;
  }
  
  p++;
  if (*p != 0) { error("malformed label: '%S': label should end with '}'.\n", label); return -1; }

  return 0;
}*/



/* text label can be either a full DELA entry, either a punc symbol, either an unknow word
 */

static int check_text_label(const unichar * label) {
const unichar * p;
if (label==NULL) {error("check label: no label\n"); return -1; }

  if (*label == 0) { error("error: check label: label is empty\n"); return -1; }

if (label[0]=='{' && label[1]!='\0') {
   /* If we have something that looks like a tag token of the form {__,__.__} */
   return (check_tag_token((unichar*)label)==1)?0:-1;
}
  /* no spaces */

  for (p = label; *p; p++) {
    if ((*p == ' ') || (*p == '\t') || (*p == '\n')) { error("malformed label: '%S'.\n", label); return -1; }
  }

  return 0;
}




void load_text_symbol(tSymbole * symb, unichar * lex) {

  int i, j;

  if (check_text_label(lex) == -1) { fatal_error("bad text label: \"%S\"\n", lex); }

  symb->sorteSymbole = ATOME;


  if (*lex == '{') {   // {__,__.__}

    /* inflected form */

    i = 1, j = 0;

    while (*(lex + i) != ',') {

      symb->flechie[j++] = lex[i++];

      if (j >= maxMot) { fatal_error("inflected form '%S' is too long.\n", lex); }
    }

    symb->flechie[j] = 0;
    i++;


    /* forme canonique */

    for (j = i ; *(lex + j) != '.'; j++);
    symb->canonique = (unichar *) malloc((j + 1 - i) * sizeof(unichar));

    j = 0 ;
    while (*(lex + i) != '.') { symb->canonique[j++] = lex[i++]; }

    symb->canonique[j] = 0;
    i++;


    /* traits gramma ... */

    j = 0;
    while (lex[i] && lex[i] != '}' && j < maxGramm - 1) { symb->gramm[j++] = lex[i++]; }
    symb->gramm[j] = 0;


  } else {    /* mot inconnu dans un texte ou ponctuation (pas d'accolade, pas de point, pas de virgule) */


    if (lex[0] == '\\') {

      if (! lex[1] || lex[2]) { fatal_error("illegal label '%S'\n", lex); }

      *symb->flechie  = 0;
      symb->canonique = u_strdup(lex);

      u_strcpy(symb->gramm, "PNC");


    } else if (u_strchr(PONCTAB, *lex)) {

      if (lex[1]) { fatal_error("illegal label text: '%S'\n", symb->flechie); }

      *symb->flechie  = 0;
      symb->canonique = u_strdup(lex);

      u_strcpy(symb->gramm, "PNC");


    } else if (u_is_digit(*lex)) { /* chiffre arabe */

      if (lex[1]) { fatal_error("illegal label text: '%S'\n", symb->flechie); }

      *symb->flechie  = 0;
      symb->canonique = u_strdup(lex);

      u_strcpy(symb->gramm, "CHFA");


    } else { // mot inconnu

      i = 0;
      while (lex[i]) {
   if (i >= maxMot) { fatal_error("inflected form too long in '%S'.\n", lex); }
   symb->flechie[i] = lex[i];
   i++;
      }
      symb->flechie[i] = 0;

      symb->canonique    = (unichar *) xmalloc(sizeof(unichar));
      symb->canonique[0] = 0;

      symb->gramm[0] = '?';
      symb->gramm[1] = 0;
    }
  }
}


#if 0


void load_gramm_symbol(tSymbole * symb, unichar * _lex) {

  int i, j;
  unichar * p, * delim;

  unichar * lex = u_strdup(_lex);

  //  debug("load_gramm_symbol: \"%S\"\n", lex);

  if (*lex == '{') {   /* dictionnary entry */

    if (u_strcmp(lex, "{S}") == 0) {
      
      symb->sorteSymbole = ATOME;
      *symb->flechie     = 0;
      symb->canonique    = lex;
      u_strcpy(symb->gramm, "PNC");
      return;
    }


    if (check_dic_entry(lex) == -1) { fatal_error("bad label grammar '%S'\n", lex); }

    symb->sorteSymbole = ATOME;

    /* inflected form */

    i = 1, j = 0;

    while (lex[i] != ',') {

      if (j >= maxMot) { fatal_error("inflected form too long in '%S'\n", lex); }

      symb->flechie[j++] = lex[i++];
    }

    symb->flechie[j] = 0;
    i++;


    /* forme canonique */

    for (j = i; lex[j] != '.'; j++);
    lex[j] = 0;

    symb->canonique = u_strdup(lex + i);

    i = j + 1;


    /* traits gramma ... */

    j = 0;
    while (lex[i] && lex[i] != '}' && j < maxGramm - 1) { symb->gramm[j++] = lex[i++]; }
    symb->gramm[j] = 0;

    free(lex);
    return;
  }


  if (*lex ==  '<') {

    if (lex[1] == 0) {
      symb->sorteSymbole = ATOME;
      *symb->flechie  = 0;
      symb->canonique = lex;
      u_strcpy(symb->gramm, "PNC");
      return;
    }

    if (u_strcmp(lex, "<def>") == 0) { /* special <def> label */
      symb->sorteSymbole = SPECIAL;
      symb->canonique    = lex;
      u_strcpy(symb->flechie, lex);
      symb->gramm[0] = 0;
      return;
    }

    if (u_strcmp(lex, "<.>") == 0) { /* UNIVERSEL label */

      symb->sorteSymbole = UNIVERSEL;
      symb->canonique    = u_strdup("");
      symb->flechie[0]   = 0;
      symb->gramm[0]     = 0;
      free(lex);
      return;
    }


    *symb->flechie = 0;

    p = lex + 1;

    if ((delim = u_strchr(p, '.'))) { /* read canonical form */

      *delim = 0;
      symb->canonique = u_strdup(p);
      p = delim + 1;

    } else {

      symb->canonique  = (unichar *) xmalloc(sizeof(unichar));
      *symb->canonique = 0;
    }

    i = 0;

    while ((*p != '>') && (i < maxGramm - 1)) {

      if (*p == 0) { fatal_error("missing closing '>' in label: '%S'\n", _lex); }

      symb->gramm[i++] = *(p++);
    }
    symb->gramm[i] = 0;


    symb->sorteSymbole = INDETERMINE;

    free(lex);
    return;
  }



  /* label doesn't start with '{' nor '<' */


  if ((*lex == '!') || (*lex == '=')) { /* special rule's symbols */

    if (lex[1]) { fatal_error("bad grammar label: '%S'\n", lex); }

    symb->sorteSymbole = ATOME;
    symb->canonique    = (unichar *) xmalloc(sizeof(unichar));
    symb->canonique[0] = 0;
    u_strcpy(symb->flechie, lex);
    u_strcpy(symb->gramm, lex);

    free(lex);

    return;
  }



  /* mot inconnu dans un texte ou ponctuation (pas d'accolade, pas de point, pas de virgule) */


  symb->sorteSymbole = ATOME;
  *symb->flechie = 0;

  if (*lex == '\\') {

    if (! lex[1] || lex[2]) { fatal_error("illegal label '%S'\n", *lex); }

    symb->canonique = lex;
    u_strcpy(symb->gramm, "PNC");    
    return;
  }


  if (u_strchr(PONCTAB, *lex)) {     /* ponctuation */

    if (lex[1]) { fatal_error("bad grammar label: '%S'\n", lex); }

    symb->canonique = lex;
    u_strcpy(symb->gramm, "PNC");
    return;
  }

 
  if (u_is_digit(*lex)) { // chiffre arabe

    if (lex[1]) { fatal_error("illegal label text: '%S'\n", lex); }

    symb->canonique = lex;
    u_strcpy(symb->gramm, "CHFA");
    return;
  }


  /* mot inconnu */

  i = 0;
  while (lex[i]) {
    if (i >= maxMot) { fatal_error("inflected form too long: %S Y.\n", lex); }
    symb->flechie[i] = lex[i];
    i++;
  }
  symb->flechie[i] = 0;


  symb->sorteSymbole = ATOME;

  symb->canonique    = (unichar *) xmalloc(sizeof(unichar));
  symb->canonique[0] = 0;

  symb->gramm[0] = '?';
  symb->gramm[1] = 0;

  free(lex);
}


tAutAlMot * fst2AutAlMot(Fst2 * automaton, int nb) {

  nb++; /* Fst2 start at 1 */

  tAutAlMot * aut = initAutAlMot(automaton->number_of_states_per_graphs[nb]);

  tAlphMot * alphabet = new_SymbolAlphabet();

  int base = automaton->initial_states[nb];

  for (int q = 0; q < (int) aut->nbEtats; q++) {

    int qq = base + q;

    aut->type[q] = 0;


    if (is_final_state(automaton->states[qq]->control))  { aut->type[q] |= AUT_FINAL;   }
    if (automaton->states[qq]->control & FST2_INITIAL_STATE_BIT_MASK)  { aut->type[q] |= AUT_INITIAL; }

    aut->etats[q] = NULL;

    for (struct fst2Transition * transitions = automaton->states[qq]->transitions;
         transitions; transitions = transitions->next) {

      tSymbole symb;

      load_text_symbol(& symb, automaton->tag_number[transitions->tag_number]->input);

      alphabet_clear(alphabet);

      int nb_inflect = symbole_developp(alphabet, & symb);

      if (nb_inflect == 0) { fatal_error("symbole_developp: nbflex=0\n"); }

      while (nb_inflect--) {
	nouvTrans(aut, q, alphabet->symb + nb_inflect, transitions->state_number - base);
        free_Ustring(alphabet->symb[nb_inflect].flex);
	free(alphabet->symb[nb_inflect].canonique);
      }
    }
  }

  free_SymbolAlphabet(alphabet);

  return aut;
}


#endif



list_aut_old * load_text_automaton(char * fname, bool developp) {


  //unichar buf[1024];

  u_printf("Loading text automaton...\n");
  Fst2 * A = load_fst2(fname, 1);
  if (A == NULL) { fatal_error("cannot load %s\n", fname); }

  list_aut_old * res = (list_aut_old *) xmalloc(sizeof(list_aut_old));

  res->nb_aut    = A->number_of_graphs;
  res->les_aut   = (tAutAlMot **) xmalloc(sizeof(tAutAlMot *) * res->nb_aut);

  tAlphMot * alphabet = alphabet_new();

  for (int i = 0; i < res->nb_aut; i++) {

    int nb   = i + 1;  /* automate in fst2 start at index 1 */
    int base = A->initial_states[nb];

    tAutAlMot * aut = initAutAlMot(A->number_of_states_per_graphs[nb]);

    aut->name = u_strdup(A->graph_names[nb]);

    for (int q = 0; q < (int) aut->nbEtats; q++) {

      int qq = base + q;

      aut->type[q] = 0;

      if (is_final_state(A->states[qq])) { aut->type[q] |= AUT_TERMINAL; }
      if (is_initial_state(A->states[qq])) { aut->type[q] |= AUT_INITIAL;  }

      aut->etats[q] = NULL;

      for (Transition * trans = A->states[qq]->transitions; trans; trans = trans->next) {

	//tSymbole symb;

        tSymbole * symb = tSymbole_new();

	//u_strcpy(buf, A->tags[trans->tag_number]->input);
        //        debug("in=%S\n", A->tags[trans->tag_number]->input);
	load_text_symbol(symb, A->tags[trans->tag_number]->input);
        //debug("out=(%d:%S,%S.%S)\n", symb->sorteSymbole, symb->flex->str, symb->canonique, symb->gramm);

        if (developp) { // developp symbols

          alphabet_clear(alphabet);

          int nbflex = symbole_developp(alphabet, symb);

          if (nbflex == 0) { nouvTrans(aut, q, NULL, trans->state_number - base); }

          while (nbflex--) {
            nouvTrans(aut, q, alphabet->symb + nbflex, trans->state_number - base);
            free(alphabet->symb[nbflex].canonique);
            alphabet->symb[nbflex].canonique = NULL;
            free_Ustring(alphabet->symb[nbflex].flex);
            alphabet->symb[nbflex].flex  = NULL;
          }

        } else { nouvTrans(aut, q, symb, trans->state_number - base); }
        
        symbole_delete(symb);
      }
    }
    res->les_aut[i] = aut;
  }


  free_SymbolAlphabet(alphabet);
  free_Fst2(A);

  return res;
}



#if 0

tAutAlMot * load_elag_grammar_automaton(char * fname) {


  unichar buf[1024];

  Fst2 * automaton = load_fst2(fname, 1);

  if (automaton == NULL) { fatal_error("cannot load %s\n", fname); }

  if (automaton->number_of_graphs != 1) { error("%d graphs in grammar %s\n", automaton->number_of_graphs, fname); }

  int nb   = 1;  /* automate in fst2 start at index 1 */
  int base = automaton->initial_states[nb];


  tAutAlMot * aut = initAutAlMot(automaton->number_of_states_per_graphs[1]);

  aut->name = u_strdup(automaton->graph_names[nb]);

  tAlphMot * alphabet = new_SymbolAlphabet();


  for (int q = 0; q < (int) aut->nbEtats; q++) {

    int qq = base + q;

    aut->type[q] = 0;

    if (is_final_state(automaton->states[qq])) { aut->type[q] |= AUT_TERMINAL; }
    if (automaton->states[qq]->control & FST2_INITIAL_STATE_BIT_MASK) { aut->type[q] |= AUT_INITIAL;  }

    aut->etats[q] = NULL;

    for (struct fst2Transition * transitions = automaton->states[qq]->transitions; transitions; transitions = transitions->next) {

      tSymbole symb;

      u_strcpy(buf, automaton->tag_number[transitions->tag_number]->input);

      load_gramm_symbol(& symb, buf);

      alphabet_clear(alphabet);

      int nb_inflect = symbole_developp(alphabet, & symb);

      if (nb_inflect == 0) { nouvTrans(aut, q, NULL, transitions->state_number - base); }

      while (nb_inflect--) {
	//#warning "must free canonique!!!!????"
	nouvTrans(aut, q, alphabet->symb + nb_inflect, transitions->state_number - base);
	free(alphabet->symb[nb_inflect].canonique);
      }

    }
  }

  free_SymbolAlphabet(alphabet);
  free_Fst2(automaton);

  return aut;
}

#endif


void text_output_fst2(list_aut_old * txt, FILE * f) {

  unichar buf[1024];
  u_strcpy(buf, "0000000000");

  int i = 9;
  int n = txt->nb_aut;

  while (n) {
    buf[i--] = '0' + (n % 10);
    n = n / 10;
  }

  u_fprintf(f, "%S\n", buf);


  string_hash * hash = new_string_hash();


  /* add epsilon before all other labels */

  u_strcpy(buf, "<E>");

  get_value_index(buf, hash);


  for (i = 0; i < txt->nb_aut; i++) {

    tAutAlMot * A = txt->les_aut[i];

    u_fprintf(f, "-%d %S\n", i + 1, A->name);

    for (int q = 0; q < A->nbEtats; q++) {

      u_fputc((A->type[q] & AUT_TERMINAL) ? 't' : ':', f);
      u_fputc(' ', f);

      for (tTransitions * t = A->etats[q]; t; t = t->suivant) {

	if (t->etiq == NULL) {

	  error("<def> trans on a txt automaton???\n");
	  u_strcpy(buf, "<def>");

	} else {

	  switch (t->etiq->sorteSymbole) {
	    
	  case ATOME:

	    //if (*t->etiq->flechie == 0) { // PNC, CHFA ...

            if (t->etiq->flex->len == 0) {
	      u_sprintf(buf, "%S", t->etiq->canonique);

	    } else if (*t->etiq->gramm == '?') { // unknow word
	      
	      u_sprintf(buf, "%S", t->etiq->flex->str);

	    } else {

	      u_sprintf(buf, "{%S,%S.%S}", t->etiq->flex->str, t->etiq->canonique, t->etiq->gramm);
	    }
	    break;

	  case SPECIAL:
	    error("SPECIAL label in text automaton???\n");
	    u_strcpy(buf, t->etiq->flex->str);
	    break;

	  case UNIVERSEL:
	    error("<.> label in text automaton???\n");
	    u_sprintf(buf, "<.>");
	    break;

	  case CODE_POUET:
	    error("CODE_POUET in text automaton???\n");
	    u_sprintf(buf, "<%S>", t->etiq->gramm);
	    break;

	  case NEGATIF:
	  case INCOMPLET:
	    error("INCOMPLET label in text automaton???\n");
	    u_sprintf(buf, "<%S.%S>", t->etiq->canonique, t->etiq->gramm);
	    break;

	  case INDETERMINE:
	    error("ouptut_fst2: symbol code is INDETERMINE\n", t->etiq->sorteSymbole);
	    u_sprintf(buf, "<INDETERMINE:%S,%S.%S>", t->etiq->flex->str, t->etiq->canonique, t->etiq->gramm);
	  }
	}

	u_fprintf(f, "%d %d ", get_value_index(buf, hash), t->but);
      }

      u_fputc('\n', f);
    }

    u_fprintf(f,"f \n");
  }
  output_fst2_labels(hash, f);
  free_string_hash(hash);
}



int text_output_fst2_fname(list_aut_old * txt, char * fname) {

  FILE * f = u_fopen(fname, U_WRITE);

  if (f == NULL) { error("output_fst2: unable to open %s for writing\n", fname); return -1; }

  text_output_fst2(txt, f);

  fclose(f);

  return 0;
}