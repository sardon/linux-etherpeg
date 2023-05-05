/* $Id: main.c,v 1.3 2002/06/22 16:57:37 seb Exp $ */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <unistd.h>
#include <gdk_imlib.h>
#include <pthread.h>

#include "interface.h"
#include "support.h"
#include "sort_frame.h"
#include "promiscuity.h"


#define MAX_IMAGES 300

static volatile int counter = 0;
GtkWidget *mainWindow;
guint statusBarContextId = 0;


GtkWidget *viewport;
GtkWidget *statusbar;
GtkWidget *fixed1;

GtkWidget *createPixMapInWindow(GdkPixmap *gdkPixmap){
  static GtkWidget *images[MAX_IMAGES];
  static int cpt = 0;
  guint x, y;
  
  GtkWidget *pixmap = NULL;

  if (images[cpt]!=NULL)
    gtk_widget_destroy(images[cpt]); // free up the image

  images[cpt] = gtk_pixmap_new(gdkPixmap,NULL);
  pixmap = images[cpt];
  cpt=(cpt++)%(MAX_IMAGES);
  gtk_widget_ref (pixmap);
  gtk_object_set_data_full (GTK_OBJECT (mainWindow), "pixmap", pixmap, 
			    (GtkDestroyNotify) gtk_widget_unref); 

  //find coordinates
  x = (abs(rand())%(viewport->allocation.width-pixmap->requisition.width));
  y = (abs(rand())%(viewport->allocation.height-pixmap->requisition.height));
  gtk_fixed_put((GtkFixed *)fixed1,pixmap,x,y);
  gtk_pixmap_set_build_insensitive (GTK_PIXMAP (pixmap), FALSE);

  return(pixmap);
}



void displayJPEG(char *filename){
  /*GdkImlibImage         *im;*/
  /*gint w,h;*/
  GdkPixmap             *gdkPixmap;
  GdkBitmap             *mask;
  GtkWidget             *pixmap;

  //im=gdk_imlib_load_image(filename);

  /* get GTK thread lock */
  gdk_threads_enter ();
  if (gdk_imlib_load_file_to_pixmap(filename, &gdkPixmap, &mask)!=1){
    fprintf(stderr,"Could not load file (corrupt?) - skipping");
  } else {
    pixmap = createPixMapInWindow(gdkPixmap);
    gtk_widget_show (pixmap);
  }
  gdk_threads_leave();
    
}


void statusBarPrint(char *s){
  gtk_statusbar_push((GtkStatusbar *)statusbar,statusBarContextId,(const gchar *)s);
}


/* this is a callback function from the promiscuity code */
void DisplayImageAndDisposeHandle(char * jpeg, int size){
  static char gifSentinel[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
  static char jpegSentinel[] = {0xFF,0xD9,0xFF,0xD9,0xFF,0xD9,0xFF,0xD9,0xFF,0xD9,0xFF,0xD9,0xFF,0xD9,0xFF,0xD9};

  //write into a file for now
  char filename[2000]; //YIRKS this is bad
  char *ext;
  char *GIFExt=".gif";
  char *JPEGExt=".jpg";
  char *dataToFree = jpeg;
  FILE *fp;

  char *sentinel;
  int sentinelSize, t;
  int bwritten = 0;
	
  if(jpeg == NULL) return;
	
 again:
  if( 'G' == *jpeg) {
    //grip = gripG;
    // for GIF:
    // FF FF FF FF will ensure that the bit parser aborts, since you can't
    // have two consecutive all-ones symbols in the LZW codestream --
    // you can sometimes have one (say, a 9-bit code), but after consuming
    // it the code width increases (so we're now using 10-bit codes) 
    // and the all-ones code won't be valid for a while yet.
    sentinel = gifSentinel;
    sentinelSize = sizeof(gifSentinel);
    ext=GIFExt;

  }
  else {
    //grip = gripJ;
    // for JPEG:
    // FF D9 FF D9 will ensure (a) that the bit-parser aborts, since FF D9
    // is an "unstuffed" FF and hence illegal in the entropy-coded datastream,
    // and (b) be long enough to stop overreads.
    sentinel = jpegSentinel;
    sentinelSize = sizeof(jpegSentinel);
    ext=JPEGExt;

  }
	
  //ее add sentinel pattern to the end of the handle.
  jpeg  = (char *)realloc(jpeg,size+sentinelSize);
  memcpy(jpeg+size,sentinel,sentinelSize);
  dataToFree = jpeg;
  size+=sentinelSize;
  
  // go through a temp file - 
  // this absolutely ugly but I don't know how to use imlib otherwise - please help!
  sprintf(filename,"file_%d%s",counter++,ext);
  if ((fp=fopen(filename,"w"))==NULL){
    perror("Could not create file");
    return;
  }
   bwritten = 0;
   while(bwritten<size){
    if ((t=fwrite(jpeg,1,size,fp))<=0){
      perror("write");
      break;
    }
    bwritten+=t;
  }
  fclose(fp);
  sync(); //yuk
  displayJPEG(filename);
  unlink(filename); //delete tempfile after displaying

	
  if(scanForAnotherImageMarker(jpeg,size) ) {
    printf("again!\n");
    goto again;
  }

  free(dataToFree);
  
}

void showBlob(short n ){
  switch (n){
  case 0: // yellow
    //statusBarPrint("Packet discarded\n");
    break;
  case 1:
    //printf("uninteresting packet read\n");
    break;
  case 2:
    //statusBarPrint("Interesting packet read\n");
    break;
  case 3:
    //statusBarPrint("whole image read, writing to file\n");
    break;
  }
}


void eraseBlobArea(void ){
  
}


void *captureThread(void *a){
  while (1){
    idlePromiscuity(); 
  } 
}


void quit_cb(void){
  termPromiscuity();
  exit(0);
}

void usage(char *p){
  fprintf(stderr,"Usage: %s [-i <interface name>]\n",p);
}


int
main (int argc, char *argv[])
{

  int a = 0;
  char c; 
  char *ifaceName; 


  pthread_t capture_tid;

  /* init threads */
  g_thread_init (NULL);
  //gdk_threads_init ();
  gdk_init(&argc,&argv);
  gdk_imlib_init();

  
  /* init gtk */
  gtk_set_locale ();
  gtk_init(&argc, &argv);
  gtk_widget_push_visual(gdk_imlib_get_visual());
  gtk_widget_push_colormap(gdk_imlib_get_colormap());

  /* create window */
  mainWindow = create_mainWindow ();
  /* find our widget */
  statusbar = lookup_widget(mainWindow,"statusbar");
  viewport = lookup_widget(mainWindow,"viewport");
  fixed1 = lookup_widget(mainWindow,"fixed1");
  statusBarContextId = gtk_statusbar_get_context_id((GtkStatusbar *)statusbar,"info");

  /* Handle window manager closing */
  gtk_signal_connect(GTK_OBJECT(mainWindow), "delete_event", 
   		     GTK_SIGNAL_FUNC(quit_cb), NULL); 
  gtk_widget_show (mainWindow);


  /* parse command-line args */
  ifaceName = DEFAULT_IFACE;
  while ((c = getopt(argc, argv, "i:")) != -1) {
    switch (c) {
    case 'i':	
      ifaceName = optarg;
      break;
    case '?':
    default:
      usage(argv[0]);
      exit(1);
    }
  }

  /* init our stuff */
  createStash();
  initPromiscuity(ifaceName);

  /* create our activity thread */
  pthread_create (&capture_tid, NULL, captureThread, (void *)&a);

  /* enter the GTK main loop */
  gdk_threads_enter ();
  gtk_main ();
  gdk_threads_leave ();
  
  return 0;
}

