#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <hash.h>
#include <list.h>
#include <util.h>


typedef enum {OPEN , STOP , SHUT} well_state_type;

static const double RATE_ERROR = -1.0;
static const char *wconhist_string = "WCONHIST";
static const char *dates_string    = "DATES";
static const char *slash_string    = "/";

static void rate_node_free__(void *);

/*****************************************************************/

typedef struct {
  char            *well;
  /*char            *control_mode;*/
  well_state_type  state;   
  double 	   ORAT;    
  double 	   WRAT;    
  double 	   GRAT;    
  double 	   THP;	    
  double 	   BHP;	    
  double 	   GOR;	    
  double 	   WCT;     
} rate_node_type;


typedef struct {
  int        date_nr;
  char      *date_string;
  list_type *rates;
} date_node_type;



static date_node_type * date_node_alloc(int date_nr) {
  date_node_type * date_node = malloc(sizeof *date_node);
  date_node->date_nr     = date_nr;
  date_node->rates       = list_alloc();
  date_node->date_string = NULL;
  return date_node;
}


static void date_node_free(date_node_type *date_node) {
  list_free(date_node->rates);
  if (date_node->date_string != NULL) free(date_node->date_string);
  free(date_node);
}


static void date_node_free__(void  *__date_node) {
  date_node_type *date_node = (date_node_type *) __date_node;
  date_node_free(date_node);
}


/*
  Her kan det v�re et vilk�rlig filter ... 
*/

static void date_node_add_rate(date_node_type * date_node , const rate_node_type *rate_node) {
  if (rate_node->ORAT > 0.0)
    list_append_managed_ref(date_node->rates , rate_node , rate_node_free__);
}


static void date_node_set_date_string(date_node_type * date_node, const char **token_list) {
  date_node->date_string = malloc(strlen(token_list[0]) + strlen(token_list[1]) + strlen(token_list[2]) + 4);
  sprintf(date_node->date_string , "%s. %s %s" , token_list[0] , token_list[1] , token_list[2]);
}



/*****************************************************************/
#define COND_ATOF(s) (s == NULL) ? RATE_ERROR : atof(s)
static void rate_node_set(rate_node_type * node , int tokens , const char **token_list , bool *well_shut) {
  node->well = util_realloc_string_copy(node->well , token_list[0]);
  
  /*
    Must check for NULL here ....
  */
  node->ORAT = COND_ATOF(token_list[3]);
  node->WRAT = COND_ATOF(token_list[4]);
  node->GRAT = COND_ATOF(token_list[5]);
  node->THP  = COND_ATOF(token_list[8]);
  node->BHP  = COND_ATOF(token_list[9]);
  

  if (node->ORAT != 0.0)
    node->GOR = node->GRAT / node->ORAT;
  else
    node->GOR = RATE_ERROR;

  if ((node->ORAT + node->WRAT) != 0.0)
    node->WCT = node->WRAT / (node->ORAT + node->WRAT);
  else
    node->WCT = RATE_ERROR;
  
  if (strcmp(token_list[1] , "OPEN") == 0) node->state = OPEN;
  if (strcmp(token_list[1] , "STOP") == 0) node->state = STOP;
  if (strcmp(token_list[1] , "SHUT") == 0) node->state = SHUT;

  
}
#undef COND_ATOF





static rate_node_type * rate_node_alloc(int tokens, const char **token_list) {
  rate_node_type *node = malloc(sizeof *node);
  bool well_shut;
  node->well = NULL;
  rate_node_set(node , tokens , token_list , &well_shut);
  return node;
}


static void rate_node_free(rate_node_type *rate_node) {
  free(rate_node->well);
  free(rate_node);
}


static void rate_node_free__(void *__rate_node) {
  rate_node_type *rate_node = (rate_node_type *) __rate_node;
  rate_node_free(rate_node);
}


/*
static rate_node_type * rate_node_copyc(const rate_node_type *src) {
  rate_node_type *new = malloc(sizeof *new);
  new->well  = strdup(src->well);
  new->state = src->state;   
  new->ORAT  = src->ORAT;    
  new->WRAT  = src->WRAT;    
  new->GRAT  = src->GRAT;    
  new->THP   = src->THP;	    
  new->BHP   = src->BHP;	    
  new->GOR   = src->GOR;	    
  new->WCT   = src->WCT;     

  return new;
}

static void * rate_node_copyc__(const void *__src) {
  rate_node_type *src = (rate_node_type *) __src;
  return rate_node_copyc(src);
}
*/

static void rate_node_fprintf(const rate_node_type *rate_node , FILE *stream) {
  /*if (rate_node->ORAT > 0.0)*/
  fprintf(stream , "%-8s %16.4f  %16.4f  %16.4f \n",rate_node->well , rate_node->ORAT , rate_node->WRAT , rate_node->GRAT);
}

/*****************************************************************/


static char * strdup_n(const char *s , int max_len) {
  char *new_string;
  int len;

  if (strlen(s) > max_len)
    len = max_len;
  else
    len = strlen(s);
  new_string = malloc(len + 1);
  
  strncpy(new_string , s , len);  new_string[len] = '\0';
  return new_string;
}


static void token_list_free(int size, char **token_list) {
  int i;
  for (i=0; i < size; i++)
    if (token_list[i] != NULL) free(token_list[i]);
  
  free(token_list);
}



static char * strip_line_alloc(const char * line) {
  const char  comment_char = '-';
  const char *space   = " \t";
  char * new_line = NULL;
  int offset, length,pos;
  bool cont , quote_on , at_end;
  
  offset   = strspn(line , space);
  quote_on = false;
  cont     = true;
  at_end   = false;
  length   = 0;
  if (line[offset] != '\0') {
    pos = offset;
    do {
      if (line[pos] == '\'' || line[pos] == '"')
	quote_on = !quote_on;

      if (line[pos] == comment_char && !quote_on) {
	cont   = false;
	length = pos - offset;
      }
      
      if (pos == (strlen(line) - 1)) {
	length = pos - offset + 1;
	cont = false;
	at_end = true;
      }
      if (cont)
	pos++;
    } while (cont);

    /*
      Remove trailing space:
    */
    if (at_end) {
      while (line[offset + length - 1] == ' ')
	length--;
    }
      
  
    if (length > 0)
      new_line = strdup_n(&line[offset] , length);
  } 
  
  return new_line;
}


static char * alloc_line(FILE *stream , bool *at_eof) {
  int init_pos = ftell(stream);
  int len;
  char *line;
  char c , end_char;
  bool cont;
  bool dos_newline;
  
  len = 0;
  cont = true;
  
  
  do {
    c = fgetc(stream);
    if (c != '\n' && c != '\r' && c != EOF)
      len++;
    else
      cont = false;
  } while (cont);
  if (c == '\r')
    dos_newline = true;
  else
    dos_newline = false;
  end_char = c;

  fseek(stream , init_pos , SEEK_SET);
  {
    char *tmp_line = malloc(len + 1);
    int i;
    for (i=0; i < len; i++)
      tmp_line[i] = fgetc(stream);
    tmp_line[len] = '\0';
    line = strip_line_alloc(tmp_line);
    free(tmp_line);
  }
  fgetc(stream);
  if (dos_newline)
    fgetc(stream);
  
  if (end_char == EOF)
    *at_eof = true;
  else
    *at_eof = false;

  return line;
}


static void parse_line(const char * line , int *_tokens , char ***_token_list , int min_tokens) {
  const char *delimiters = " ";
  int    token,tokens,offset,length;
  char **token_list;
  
  tokens  = 0;
  offset = strspn(line , delimiters);

  while (line[offset ] != '\0') {
    length = strcspn(&line[offset] , delimiters);
    if (length > 0) {
      char * token_string = strdup_n(&line[offset] , length);
      char * star_ptr     = token_string;
      long int items      = strtol(token_string , &star_ptr , 10);
      
      if (star_ptr != token_string && star_ptr[0] == '*') 
	tokens += items;
      else
	tokens++;
      
      offset += length;                              /* Skipping the token */
      offset += strspn(&line[offset] , delimiters);  /* Skipping the following whitespace */

      free(token_string);
    }
  }
  
  if (tokens < min_tokens)
    tokens = min_tokens;
  token_list = malloc(tokens * sizeof *token_list);
  for (token = 0; token < tokens; token++) 
    token_list[token] = NULL;
  
  token   = 0;
  offset = strspn(line , delimiters);
  while (line[offset ] != '\0') {
    length = strcspn(&line[offset] , delimiters);
    if (length > 0) {
      char * token_string  = strdup_n(&line[offset] , length);
      char * star_ptr      = token_string;
      long int items = strtol(token_string , &star_ptr , 10);
      
      if (star_ptr != token_string && star_ptr[0] == '*') {
	int t1 = token;
	int t2 = token + items;
	    
	for (token = t1; token < t2; token++) {
	  if (strlen(star_ptr) == 1)
	    token_list[token] = NULL;
	  else
	    token_list[token] = strdup(&star_ptr[1]);
	}
      } else {
	token_list[token] = strdup_n(&line[offset] , length);
	token++;
      }
      
      offset += length;                              /* Skipping the token */
      offset += strspn(&line[offset] , delimiters);  /* Skipping the following whitespace */
      free(token_string);
    }
  }
  
  /*
    Removing quotes ...
  */
  for (token = 0; token < tokens; token++) {
    if (token_list[token] != NULL) {
      if (token_list[token][0] == '\'') {
	char *tmp = strdup_n(&token_list[token][1] , strlen(token_list[token]) - 2);
	free(token_list[token]);
	token_list[token] = tmp;
      }
    }
  }

  *_tokens = tokens;
  *_token_list = token_list;
}





static void parse_file(const char *filename , int *_lines , char ***_line_list) {
  int    lines;
  char **line_list;
  int    buffer_lines;
  bool   at_eof;
  FILE *stream = fopen(filename , "r");
  
  lines        = 0;
  buffer_lines = 100;
  line_list    = NULL;
  
  line_list = realloc(line_list , buffer_lines * sizeof *line_list);
  do {
    char * line = alloc_line(stream , &at_eof);
    if (line != NULL) {
      line_list[lines] = line;
      lines++;

      if (lines == buffer_lines) {
	buffer_lines *= 2;
	line_list = realloc(line_list , buffer_lines * sizeof *line_list);
      }
    }
  } while (!at_eof);
  fclose(stream);

  line_list = realloc(line_list , lines * sizeof *line_list);
  *_lines     = lines;
  *_line_list = line_list;
}






static void sched_parse_wconhist__(int lines , const char **line_list, const char *obs_path , const char *obs_file) {

#define parse_off   0
#define wconhist_on 1
#define dates_on    2

  date_node_type *date_node;
  list_type      *wconhist = list_alloc();

  int parse_state;
  int line;
  int date_nr;
  
  
  
  parse_state = parse_off;
  date_nr     = 1;
  date_node   = date_node_alloc(date_nr); 
  list_append_managed_ref(wconhist , date_node , date_node_free__);

  /*
    The reference to the date_node object is still valid - and can be 
    updated from here.
  */


  for (line = 0; line < lines; line++) {
    bool skiprest = false;

    
    if (strcmp(line_list[line] , dates_string) == 0) {
      parse_state = dates_on;
      skiprest = true;
    }

    if (strcmp(line_list[line] , wconhist_string) == 0) {
      parse_state = wconhist_on;
      skiprest = true;
    }
    
    if (strcmp(line_list[line] , slash_string) == 0) {
      if (parse_state == dates_on) {
	/*
	  We are closing a date - must add a new dates_node
	*/
	date_nr++;
	date_node = date_node_alloc(date_nr); 
	list_append_managed_ref(wconhist , date_node , date_node_free__);
      }
      parse_state = parse_off;
      skiprest = true;
    }

    if (! skiprest) {
      char **token_list;
      int tokens;
      
      
      switch (parse_state) {
      case(parse_off):
	break;
      case(dates_on):
	parse_line(line_list[line] , &tokens , &token_list , 3);
	date_node_set_date_string(date_node , (const char **) token_list);
	token_list_free(tokens , token_list);
	break;
      case(wconhist_on):
	{
	  rate_node_type *rate_node;
	  parse_line(line_list[line] , &tokens , &token_list , 10);
	  rate_node = rate_node_alloc(tokens , (const char **) token_list);
	  date_node_add_rate(date_node , rate_node);
	  token_list_free(tokens , token_list);
	}
	break;
      }
    }
  }  
  
  {
    list_node_type * list_node = list_get_head(wconhist);
    
    while(list_node != NULL) {
      char *path = malloc(strlen(obs_path) + 1 + 4 + 1);
      date_node_type * date_node = list_node_value_ptr(list_node);
      if (list_get_size(date_node->rates) > 0) {
	sprintf(path , "%s/%04d" , obs_path , date_node->date_nr);
	util_make_path(path);
	printf("Making directory: %s \n",path );
	{
	  list_node_type * rate_list_node = list_get_head(date_node->rates);
	  if (rate_list_node != NULL) {
	    FILE * stream;
	    char * file = malloc(strlen(path) + 2 + strlen(obs_file));
	    sprintf(file , "%s/%s" , path , obs_file);
	    stream = fopen(file , "w");
	    fprintf(stream , "%d\n",list_get_size(date_node->rates));
	    while (rate_list_node != NULL) {
	      rate_node_fprintf(list_node_value_ptr(rate_list_node) , stream);
	      rate_list_node = list_node_get_next(rate_list_node);
	    }
	    fclose(stream);
	    free(file);
	  }
	}
      }
      list_node = list_node_get_next(list_node);
      free(path);
    }
  }

  list_free(wconhist);
}


/*****************************************************************/


void sched_parse_wconhist(const char *filename , const char *obs_path , const char *obs_file) {
  int lines;
  char **line_list;
  parse_file(filename , &lines , &line_list);
  sched_parse_wconhist__(lines , (const char **) line_list , obs_path , obs_file);
  util_free_string_list(line_list , lines);
}
  

/*
  int main(void) {
  sched_parse_wconhist("SCHEDULE_orig.INC" , "Observations" , "PROD");
  return 0;
  }
*/
