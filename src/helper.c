/*
 * parse: parse simple name/value pairs
 *
 * SAMPLE BUILD:
 * cc -g -Wall -o parse parse.c
 *
 * SAMPLE OUTPUT:
 * ./parse =>
 *   Initializing parameters to default values...
 *   Reading config file...
 *   Final values:
 *     item: cone, flavor: vanilla, size: large
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAXLEN 80
#define CONFIG_FILE "sample.txt"

struct sample_parameters
{
  char item[MAXLEN];
  char flavor[MAXLEN];
  char size[MAXLEN];
}
  sample_parameters;

/*
 * initialize data to default values
 */
void
init_parameters (struct sample_parameters * parms)
{
  strncpy (parms->item, "cup", MAXLEN);
  strncpy (parms->item, "chocolate", MAXLEN);
  strncpy (parms->item, "small", MAXLEN);
}

/*
 * trim: get rid of trailing and leading whitespace...
 *       ...including the annoying "\n" from fgets()
 */
char *
trim (char * s)
{
  /* Initialize start, end pointers */
  char *s1 = s, *s2 = &s[strlen (s) - 1];

  /* Trim and delimit right side */
  while ( (isspace (*s2)) && (s2 >= s1) )
    s2--;
  *(s2+1) = '\0';

  /* Trim left side */
  while ( (isspace (*s1)) && (s1 < s2) )
    s1++;

  /* Copy finished string */
  strcpy (s, s1);
  return s;
}

/*
 * parse external parameters file
 *
 * NOTES:
 * - There are millions of ways to do this, depending on your
 *   specific needs.
 *
 * - In general:
 *   a) The client will know which parameters it's expecting
 *      (hence the "struct", with a specific set of parameters).
 *   b) The client should NOT know any specifics about the
 *      configuration file itself (for example, the client
 *      shouldn't know or care about it's name, its location,
 *      its format ... or whether or not the "configuration
 *      file" is even a file ... or a database ... or something
 *      else entirely).
 *   c) The client should initialize the parameters to reasonable
 *      defaults
 *   d) The client is responsible for validating whether the
 *      pararmeters are complete, or correct.
 */
void
parse_config (struct sample_parameters * parms)
{
  char *s, buff[256];
  FILE *fp = fopen (CONFIG_FILE, "r");
  if (fp == NULL)
  {
    return;
  }

  /* Read next line */
  while ((s = fgets (buff, sizeof buff, fp)) != NULL)
  {
    /* Skip blank lines and comments */
    if (buff[0] == '\n' || buff[0] == '#')
      continue;

    /* Parse name/value pair from line */
    char name[MAXLEN], value[MAXLEN];
    s = strtok (buff, "=");
    if (s==NULL)
      continue;
    else
      strncpy (name, s, MAXLEN);
    s = strtok (NULL, "=");
    if (s==NULL)
      continue;
    else
      strncpy (value, s, MAXLEN);
    trim (value);

    /* Copy into correct entry in parameters struct */
    if (strcmp(name, "item")==0)
      strncpy (parms->item, value, MAXLEN);
    else if (strcmp(name, "flavor")==0)
      strncpy (parms->flavor, value, MAXLEN);
    else if (strcmp(name, "size")==0)
      strncpy (parms->size, value, MAXLEN);
    else
      printf ("WARNING: %s/%s: Unknown name/value pair!\n",
        name, value);
  }

  /* Close file */
  fclose (fp);
}

/*
 * program main
 */
int
main (int argc, char *argv[])
{
  struct sample_parameters parms;

  printf ("Initializing parameters to default values...\n");
  init_parameters (&parms);

  printf ("Reading config file...\n");
  parse_config (&parms);

  printf ("Final values:\n");
  printf ("  item: %s, flavor: %s, size: %s\n",
    parms.item, parms.flavor, parms.size);

  return 0;
}

