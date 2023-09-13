// lin_chirp2.c  down chirp calculation and output waveform
// dc192kwv.dat  ---> 192k sample
// dc1megwv.dat  ---> 1M sample
// GUI version     2018.8.3

#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <string.h>
#include <math.h>

#define BUFLEN 256
#define T1M 0.000001
#define T192 0.00000521
#define VMAX 32767

typedef struct main_dialog_type {
  GtkWidget *window;
  GtkWidget *button1;
  GtkWidget *button2;
  GtkWidget *button3;
	GtkWidget *label1;
  GtkWidget *label01;
  GtkWidget *label02;
  GtkWidget *label03;
  GtkWidget *label04;
  GtkWidget *label05;
  GtkWidget *label06;

  GtkWidget *entry1; 
  GtkWidget *entry2;  GtkWidget *entry3;
  GtkWidget *label2;
  GtkWidget *vbox;
  GtkWidget *hbox;
} MainDialog;

double	fstart, fstop, pduration;

  //Calculateボタンがクリックされると動作する関数
static void cb_button_clicked1(GtkWidget *widget, gpointer data)
{
  const gchar *text;
  char text4[256];
//	int		flg =

		text = gtk_entry_get_text(GTK_ENTRY(((MainDialog*)data)->entry1));
 		sscanf(text, "%lf", &fstart);
  		printf( "start:%e\n", fstart );
 		if( fstart > 100000. || fstart < 100. )
			{
   				gtk_label_set_text(GTK_LABEL(((MainDialog*)data)->label02), "Invalid");
				
			}
		else  	{
			 sprintf(text4, "%8.4e", fstart);  
	 		 gtk_label_set_text(GTK_LABEL(((MainDialog*)data)->label02), text4);
			}

 		text = gtk_entry_get_text(GTK_ENTRY(((MainDialog*)data)->entry2));
  		sscanf(text, "%lf", &fstop);
  		printf( "stop:%e\n", fstop );
 		if( fstop > 100000. || fstop < 100. )
   				gtk_label_set_text(GTK_LABEL(((MainDialog*)data)->label04), "Invalid");
		else  	{
			 	sprintf(text4, "%8.4e", fstop);  
 		 		gtk_label_set_text(GTK_LABEL(((MainDialog*)data)->label04), text4);
				}

	  	text = gtk_entry_get_text(GTK_ENTRY(((MainDialog*)data)->entry3));
 		sscanf(text, "%lf", &pduration);
  		printf( "duration:%e\n", pduration );
 		if( pduration > 1. || pduration < 0.00001 )
			{
   				gtk_label_set_text(GTK_LABEL(((MainDialog*)data)->label06), "Invalid");
				
			}
		else  	{
			 sprintf(text4, "%8.4e", pduration);  
	 		 gtk_label_set_text(GTK_LABEL(((MainDialog*)data)->label06), text4);
			}
}


static void cb_button_clicked3(GtkWidget *button2, gpointer user_data)
{
  gtk_main_quit();
}


static void cb_button_clicked2(GtkWidget *button2, gpointer user_data)
{
  gtk_main_quit();
	exit(1);
}


int main( int argc, char *argv[])
{
FILE	*fp;
double	 pk, fai, t;
char 	buf[BUFLEN];
int		k, DLEN192, DLEN1M,duration_short;
MainDialog dialog;
char s[64];
int aa;  char **qq;
aa=0;

duration_short=atoi(argv[1]); //comandline hikisu


if( NULL == (fp = fopen("chirp.dat","r")) )
	{	fprintf(stderr, "Cannot open data file\n");	exit(0);	}
	fgets(buf, BUFLEN, fp);	fstart = atof(buf);
	fgets(buf, BUFLEN, fp);	fstop = atof(buf);
	fgets(buf, BUFLEN, fp);	pduration = atof(buf);
	
fclose(fp);

  gtk_init(&aa, &qq);

  dialog.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_size_request(dialog.window,600,400);
  gtk_window_set_title(GTK_WINDOW(dialog.window), "Pulse Compression Method");
  gtk_window_set_position(GTK_WINDOW(dialog.window), GTK_WIN_POS_CENTER);
  gtk_container_set_border_width(GTK_CONTAINER(dialog.window), 10);

  dialog.vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_add(GTK_CONTAINER(dialog.window), dialog.vbox);

  dialog.hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start(GTK_BOX(dialog.vbox), dialog.hbox, TRUE, TRUE, 0);
  dialog.label1 = gtk_label_new("Enter Parameters");
  gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.label1, TRUE, TRUE, 0);


// start freq
  dialog.hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start(GTK_BOX(dialog.vbox), dialog.hbox, TRUE, TRUE, 0);
 	dialog.label01 = gtk_label_new("Start Frequency [Hz]");
 	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.label01, TRUE, TRUE, 0);
   sprintf(s,"%8.4e", fstart);
	dialog.label02 = gtk_label_new(s);
 	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.label02, TRUE, TRUE, 0);
  	dialog.entry1 = gtk_entry_new();
  	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.entry1, TRUE, TRUE, 0);

//stop freq
  dialog.hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start(GTK_BOX(dialog.vbox), dialog.hbox, TRUE, TRUE, 0);
 	dialog.label03 = gtk_label_new("Stop Frequency [Hz]");
 	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.label03, TRUE, TRUE, 0);
   sprintf(s,"%8.4e", fstop);
	dialog.label04 = gtk_label_new(s);
 	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.label04, TRUE, TRUE, 0);
   dialog.entry2 = gtk_entry_new();
 	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.entry2, TRUE, TRUE, 0);

//duration
  dialog.hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start(GTK_BOX(dialog.vbox), dialog.hbox, TRUE, TRUE, 0);
 	dialog.label05 = gtk_label_new("     Duration [sec]     ");
 	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.label05, TRUE, TRUE, 0);
   sprintf(s,"%8.4e", pduration);
	dialog.label06 = gtk_label_new(s);
 	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.label06, TRUE, TRUE, 0);
   dialog.entry3 = gtk_entry_new();
 	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.entry3, TRUE, TRUE, 0);


//input button
  dialog.hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start(GTK_BOX(dialog.vbox), dialog.hbox, TRUE, TRUE, 0);
  dialog.button1 = gtk_button_new_with_label("Update");
  gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.button1, TRUE, TRUE, 0);
  g_signal_connect(dialog.button1, "clicked", G_CALLBACK(cb_button_clicked1), &dialog);

// Execute Botton
  dialog.hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start(GTK_BOX(dialog.vbox), dialog.hbox, TRUE, TRUE, 0);
  dialog.button3 = gtk_button_new_with_label("Execute");
  gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.button3, TRUE, TRUE, 0);
  g_signal_connect(dialog.button3, "clicked", G_CALLBACK(cb_button_clicked3), &dialog);

//Quit Botton
  dialog.hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start(GTK_BOX(dialog.vbox), dialog.hbox, TRUE, TRUE, 0);
  dialog.button2 = gtk_button_new_with_label("Quit");
  gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.button2, TRUE, TRUE, 0);
  g_signal_connect(dialog.button2, "clicked", G_CALLBACK(cb_button_clicked2), NULL);

  g_signal_connect(dialog.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
  gtk_widget_show_all(dialog.window);
  gtk_main();

// *************************************************

pk = pow ( ( fstop / fstart) , ( 1 / pduration) );

DLEN192 = pduration / T192 + 1;
DLEN1M =  pduration / T1M + 1;
fprintf(stderr, "DATALEN %d\t%d\n", DLEN192, DLEN1M);

if( NULL == (fp = fopen("dc192kwv.dat","w")) )
	{	fprintf(stderr, "Cannot open data file\n");	exit(0);	}
	

for (k=0; k<DLEN192; k++)
	{
	t = T192 * k; 
//	fai = 2. * M_PI * ( fstart * t - pk / 2. * t * t);
	fai = 2. * M_PI * ( (fstart * pow (pk , t) ) / ( log(pk) ) );
	fprintf(fp, "%d\n", (int)(sin(fai) * VMAX / 2.));
	}
	
	fclose(fp);

if( NULL == (fp = fopen("dc1megwv.dat","w")) )
	{	fprintf(stderr, "Cannot open data file\n");	exit(0);	}


for (k=0; k<DLEN1M; k++)
	{
	t = T1M * k; 
//	fai = 2. * M_PI * ( fstart * t - pk / 2. * t * t);
	fai = 2. * M_PI * ( (fstart * pow (pk , t) ) / ( log(pk) ) );
	fprintf(fp, "%d\n", (int)(sin(fai) * VMAX /2.));
	}
	
	fclose(fp);

if( NULL == (fp = fopen("chirp.dat","w")) )
	{	fprintf(stderr, "Cannot open data file\n");	exit(0);	}
	fprintf(fp, "%f\tStart Frequency[Hz]\n",fstart);
	fprintf(fp, "%f\tStop Frequency[Hz]\n",fstop);
	fprintf(fp, "%f\tPulse Duration[sec]\n",pduration);
fclose(fp);

system("./txt2wav -192000 dc192kwv.dat dc192kwv.wav");
///////////////////make short duration_///////////////////////////
pduration = pduration/duration_short; //to make short pulse
pk = pow ( ( fstop / fstart) , ( 1 / pduration) );

DLEN192 = pduration / T192 + 1;
DLEN1M =  pduration / T1M + 1;
fprintf(stderr, "DATALEN %d\t%d\n", DLEN192, DLEN1M);

if( NULL == (fp = fopen("dc192kwv_short.dat","w")) )
	{	fprintf(stderr, "Cannot open data file\n");	exit(0);	}
	

for (k=0; k<DLEN192; k++)
	{
	t = T192 * k; 
//	fai = 2. * M_PI * ( fstart * t - pk / 2. * t * t);
	fai = 2. * M_PI * ( (fstart * pow (pk , t) ) / ( log(pk) ) );
	fprintf(fp, "%d\n", (int)(sin(fai) * VMAX / 2.));
	}
	
	fclose(fp);

if( NULL == (fp = fopen("dc1megwv_short.dat","w")) )
	{	fprintf(stderr, "Cannot open data file\n");	exit(0);	}


for (k=0; k<DLEN1M; k++)
	{
	t = T1M * k; 
//	fai = 2. * M_PI * ( fstart * t - pk / 2. * t * t);
	fai = 2. * M_PI * ( (fstart * pow (pk , t) ) / ( log(pk) ) );
	fprintf(fp, "%d\n", (int)(sin(fai) * VMAX /2.));
	}
	
	fclose(fp);

if( NULL == (fp = fopen("chirp_short.dat","w")) )
	{	fprintf(stderr, "Cannot open data file\n");	exit(0);	}
	fprintf(fp, "%f\tStart Frequency[Hz]\n",fstart);
	fprintf(fp, "%f\tStop Frequency[Hz]\n",fstop);
	fprintf(fp, "%f\tPulse Duration[sec]\n",pduration);
fclose(fp);

system("./txt2wav -192000 dc192kwv_short.dat dc192kwv_short.wav");
}
