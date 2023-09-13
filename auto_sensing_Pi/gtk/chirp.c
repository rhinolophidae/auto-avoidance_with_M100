#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <string.h>
#include <math.h>

#define BUFLEN 256
#define dltt 0.000001


typedef enum chirp_type{
	LIN,
	EXP
} Ctype;

typedef struct main_dialog_type {
	int dialog_num;
	GtkWidget *chkbtn;
	GtkWidget *button1;

	GtkWidget *label01;
	GtkWidget *label02;
	GtkWidget *label03;
	GtkWidget *label04;
	GtkWidget *label05;
	GtkWidget *label06;
	GtkWidget *label07;
	GtkWidget *label08;
	GtkWidget *label09;

	GtkWidget *spinbutton01;
	GtkWidget *spinbutton02;
	GtkWidget *spinbutton03;

	GtkWidget *combo01;
	
	GtkWidget *vbox;
	GtkWidget *hbox;

	double fstart, fstop, pduration;
	Ctype type;
} MainDialog;

MainDialog dialog;
MainDialog dialog2;
int adc_rec_num;

static void cb_button_clicked1(GtkWidget *widget, gpointer data){
	const gchar *text;
	char text4[256];
	double fstart, fstop, pduration;
	Ctype type;
	gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog2.chkbtn));
	int flg = (((MainDialog*)data)->dialog_num) && active;

	fstart = gtk_spin_button_get_value(GTK_SPIN_BUTTON(((MainDialog*)data)->spinbutton01));
	((MainDialog*)data)->fstart = fstart;
	sprintf(text4, "%8.4e", fstart);
	printf("start: %e\n", fstart);
	gtk_label_set_text(GTK_LABEL(((MainDialog*)data)->label03), text4);

	fstop = gtk_spin_button_get_value(GTK_SPIN_BUTTON(((MainDialog*)data)->spinbutton02));
	((MainDialog*)data)->fstop = fstop;
	sprintf(text4, "%8.4e", fstop);
	printf("stop: %e\n", fstop);
	gtk_label_set_text(GTK_LABEL(((MainDialog*)data)->label05), text4);

	pduration = gtk_spin_button_get_value(GTK_SPIN_BUTTON(((MainDialog*)data)->spinbutton03));
	((MainDialog*)data)->pduration = pduration;
	sprintf(text4, "%8.4e", pduration);
	printf("duration: %e\n", pduration);
	gtk_label_set_text(GTK_LABEL(((MainDialog*)data)->label07), text4);

	((MainDialog*)data)->type = gtk_combo_box_get_active(GTK_COMBO_BOX(((MainDialog*)data)->combo01));
	text = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(((MainDialog*)data)->combo01));
	gtk_label_set_text(GTK_LABEL(((MainDialog*)data)->label09), text);
	printf("chirp type: %s\n", text);

	if(flg){
		gtk_toggle_button_toggled(GTK_TOGGLE_BUTTON(dialog2.chkbtn));
		gtk_toggle_button_toggled(GTK_TOGGLE_BUTTON(dialog2.chkbtn));
	}
}

static void cb_button_clicked_execute(GtkWidget *button2, gpointer user_data)
{
	gtk_main_quit();
}

static void cb_button_clicked_quit(GtkWidget *button2, gpointer user_data)
{
	gtk_main_quit();
	exit(1);
}

static void cb_button_toggled(GtkToggleButton *widget, gpointer data){
	gboolean active;
	const gchar *text;
	char text4[256];
	double fstart, fstop, pduration;
	Ctype type;

	active = gtk_toggle_button_get_active(widget);
	if(active) {
		fstart = dialog.fstart;
		dialog2.fstart = fstart;
		sprintf(text4, "%8.4e", fstart);
		gtk_label_set_text(GTK_LABEL(dialog2.label03), text4);
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(dialog2.spinbutton01), fstart, fstart);
		fstop = dialog.fstop;
		dialog2.fstop = fstop;
		sprintf(text4, "%8.4e", fstop);
		gtk_label_set_text(GTK_LABEL(dialog2.label05), text4);
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(dialog2.spinbutton02), fstop, fstop);
		pduration = dialog.pduration;
		dialog2.pduration = pduration;
		sprintf(text4, "%8.4e", pduration);
		gtk_label_set_text(GTK_LABEL(dialog2.label07), text4);
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(dialog2.spinbutton03), pduration, pduration);
		gtk_combo_box_set_active(GTK_COMBO_BOX(dialog2.combo01), dialog.type);
		text = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(dialog2.combo01));
		gtk_label_set_text(GTK_LABEL(dialog2.label09), text);
	} else {
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(dialog2.spinbutton01), 100., 100000.);
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(dialog2.spinbutton02), 100., 100000.);
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(dialog2.spinbutton03), 0.00001, (double)adc_rec_num/1000000.);
	}
}



int main(int argc, char *argv[]){
	FILE *fp;
	char buf[BUFLEN];
	GtkWidget *window;
	GtkWidget *button1;
	GtkWidget *button2;
	GtkWidget *note, *label, *box;
	char **qq;
	int aa;
	aa = 0;
	char s[64];
	gchar *ss;

	if (NULL == (fp = fopen("sensing_fpga.prm", "r"))){
		fprintf(stderr, "Cannot open sensing_fpga.prm\n");
		exit(2);
	}
	fgets(buf, BUFLEN, fp);
	fgets(buf, BUFLEN, fp);
	fgets(buf, BUFLEN, fp);
	fgets(buf, BUFLEN, fp);
	fgets(buf, BUFLEN, fp); adc_rec_num = atoi(buf);
	fclose(fp);

	if (NULL == (fp = fopen("chirp_drive.prm", "r"))){
		fprintf(stderr, "Cannot open chirp_drive.prm\n");
		exit(2);
	}
	fgets(buf, BUFLEN, fp); dialog.fstart = atof(buf);
	fgets(buf, BUFLEN, fp); dialog.fstop = atof(buf);
	fgets(buf, BUFLEN, fp); dialog.pduration = atof(buf);
	fgets(buf, BUFLEN, fp); dialog.type = atoi(buf);
	if (adc_rec_num < dialog.pduration * 1000000){ 
		dialog.pduration = (float)adc_rec_num  / 1000000.; 
	}
	fclose(fp);

	if (NULL == (fp = fopen("chirp_aux.prm", "r"))){
		fprintf(stderr, "Cannot open chirp_aux.prm\n");
		exit(2);
	}
	fgets(buf, BUFLEN, fp); dialog2.fstart = atof(buf);
	fgets(buf, BUFLEN, fp); dialog2.fstop = atof(buf);
	fgets(buf, BUFLEN, fp); dialog2.pduration = atof(buf);
	fgets(buf, BUFLEN, fp); dialog2.type = atoi(buf);
	if (adc_rec_num < dialog2.pduration * 1000000){ 
		dialog2.pduration = (float)adc_rec_num  / 1000000.;
	}
	fclose(fp);

	gtk_init(&aa, &qq);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_size_request(window, 600, 400);
	gtk_window_set_title(GTK_WINDOW(window), "Pulse Compression Method");
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_container_set_border_width(GTK_CONTAINER(window), 10);

	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_container_add(GTK_CONTAINER(window), box);

	note = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(box), note, TRUE, TRUE, 0);
	dialog.vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	label = gtk_label_new("drive");
	gtk_widget_show(label);
	gtk_notebook_append_page(GTK_NOTEBOOK(note), dialog.vbox, label);
	dialog2.vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	label = gtk_label_new("aux");
	gtk_widget_show(label);
	gtk_notebook_append_page(GTK_NOTEBOOK(note), dialog2.vbox, label);


	dialog.dialog_num = 0;
	dialog.hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(dialog.vbox), dialog.hbox, TRUE, TRUE, 0);
	dialog.label01 = gtk_label_new("Enter drive pulse parameters");
	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.label01, TRUE, TRUE, 0);	

	dialog.hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(dialog.vbox), dialog.hbox, TRUE, TRUE, 0);

	dialog.hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(dialog.vbox), dialog.hbox, TRUE, TRUE, 0);
	dialog.label02 = gtk_label_new("Start Frequency [Hz]");
	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.label02, TRUE, TRUE, 0);	
	sprintf(s, "%8.4e", dialog.fstart);
	dialog.label03 = gtk_label_new(s);
	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.label03, TRUE, TRUE, 0);	
	dialog.spinbutton01 = gtk_spin_button_new_with_range(100., 100000., 1000.);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog.spinbutton01), dialog.fstart);
	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.spinbutton01, TRUE, TRUE, 0);	

	dialog.hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(dialog.vbox), dialog.hbox, TRUE, TRUE, 0);
	dialog.label04 = gtk_label_new("Stop Frequency [Hz]");
	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.label04, TRUE, TRUE, 0);	
	sprintf(s, "%8.4e", dialog.fstop);
	dialog.label05 = gtk_label_new(s);
	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.label05, TRUE, TRUE, 0);	
	dialog.spinbutton02 = gtk_spin_button_new_with_range(100., 100000., 1000.);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog.spinbutton02), dialog.fstop);
	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.spinbutton02, TRUE, TRUE, 0);	

	dialog.hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(dialog.vbox), dialog.hbox, TRUE, TRUE, 0);
	sprintf(s, "Duration [sec] < %.6f", (float)adc_rec_num/1000000.);
	dialog.label06 = gtk_label_new(s);
	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.label06, TRUE, TRUE, 0);	
	sprintf(s, "%8.4e", dialog.pduration);
	dialog.label07 = gtk_label_new(s);
	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.label07, TRUE, TRUE, 0);	
	dialog.spinbutton03 = gtk_spin_button_new_with_range(0.00001, (double)adc_rec_num/1000000., dltt);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog.spinbutton03), dialog.pduration);
	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.spinbutton03, TRUE, TRUE, 0);	

	dialog.hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(dialog.vbox), dialog.hbox, TRUE, TRUE, 0);
	dialog.label08 = gtk_label_new("Chirp type");
	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.label08, TRUE, TRUE, 0);	
	dialog.label09 = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.label09, TRUE, TRUE, 0);	
	dialog.combo01 = gtk_combo_box_text_new();
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(dialog.combo01), NULL, "Lin");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(dialog.combo01), NULL, "Exp");
	gtk_combo_box_set_active(GTK_COMBO_BOX(dialog.combo01), dialog.type);
	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.combo01, TRUE, TRUE, 0);	
	ss = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(dialog.combo01));
	gtk_label_set_text(GTK_LABEL(dialog.label09), ss); 

	dialog.hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(dialog.vbox), dialog.hbox, TRUE, TRUE, 0);
	dialog.button1 = gtk_button_new_with_label("Update");
	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.button1, TRUE, TRUE, 0);
	g_signal_connect(dialog.button1, "clicked", G_CALLBACK(cb_button_clicked1), &dialog);


	dialog.dialog_num = 1;
	dialog2.hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(dialog2.vbox), dialog2.hbox, TRUE, TRUE, 0);
	dialog2.chkbtn = gtk_check_button_new_with_label("Same parameter as driving pulse");
	gtk_box_pack_start(GTK_BOX(dialog2.hbox), dialog2.chkbtn, TRUE, TRUE, 0);	
	g_signal_connect(dialog2.chkbtn, "toggled", G_CALLBACK(cb_button_toggled), &dialog2);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog2.chkbtn), TRUE);

	dialog2.hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(dialog2.vbox), dialog2.hbox, TRUE, TRUE, 0);

	dialog2.hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(dialog2.vbox), dialog2.hbox, TRUE, TRUE, 0);
	dialog2.label02 = gtk_label_new("Start Frequency [Hz]");
	gtk_box_pack_start(GTK_BOX(dialog2.hbox), dialog2.label02, TRUE, TRUE, 0);	
	sprintf(s, "%8.4e", dialog2.fstart);
	dialog2.label03 = gtk_label_new(s);
	gtk_box_pack_start(GTK_BOX(dialog2.hbox), dialog2.label03, TRUE, TRUE, 0);	
	dialog2.spinbutton01 = gtk_spin_button_new_with_range(100., 100000., 1000.);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog2.spinbutton01), dialog2.fstart);
	gtk_box_pack_start(GTK_BOX(dialog2.hbox), dialog2.spinbutton01, TRUE, TRUE, 0);	

	dialog2.hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(dialog2.vbox), dialog2.hbox, TRUE, TRUE, 0);
	dialog2.label04 = gtk_label_new("Stop Frequency [Hz]");
	gtk_box_pack_start(GTK_BOX(dialog2.hbox), dialog2.label04, TRUE, TRUE, 0);	
	sprintf(s, "%8.4e", dialog2.fstop);
	dialog2.label05 = gtk_label_new(s);
	gtk_box_pack_start(GTK_BOX(dialog2.hbox), dialog2.label05, TRUE, TRUE, 0);	
	dialog2.spinbutton02 = gtk_spin_button_new_with_range(100., 100000., 1000.);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog2.spinbutton02), dialog2.fstop);
	gtk_box_pack_start(GTK_BOX(dialog2.hbox), dialog2.spinbutton02, TRUE, TRUE, 0);	

	dialog2.hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(dialog2.vbox), dialog2.hbox, TRUE, TRUE, 0);
	sprintf(s, "Duration [sec] < %.6f", (float)adc_rec_num/1000000.);
	dialog2.label06 = gtk_label_new(s);
	gtk_box_pack_start(GTK_BOX(dialog2.hbox), dialog2.label06, TRUE, TRUE, 0);	
	sprintf(s, "%8.4e", dialog2.pduration);
	dialog2.label07 = gtk_label_new(s);
	gtk_box_pack_start(GTK_BOX(dialog2.hbox), dialog2.label07, TRUE, TRUE, 0);	
	dialog2.spinbutton03 = gtk_spin_button_new_with_range(0.00001, (double)adc_rec_num/1000000., dltt);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog2.spinbutton03), dialog2.pduration);
	gtk_box_pack_start(GTK_BOX(dialog2.hbox), dialog2.spinbutton03, TRUE, TRUE, 0);	

	dialog2.hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(dialog2.vbox), dialog2.hbox, TRUE, TRUE, 0);
	dialog2.label08 = gtk_label_new("Chirp type");
	gtk_box_pack_start(GTK_BOX(dialog2.hbox), dialog2.label08, TRUE, TRUE, 0);	
	dialog2.label09 = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(dialog2.hbox), dialog2.label09, TRUE, TRUE, 0);	
	dialog2.combo01 = gtk_combo_box_text_new();
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(dialog2.combo01), NULL, "Lin");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(dialog2.combo01), NULL, "Exp");
	gtk_combo_box_set_active(GTK_COMBO_BOX(dialog2.combo01), dialog2.type);
	gtk_box_pack_start(GTK_BOX(dialog2.hbox), dialog2.combo01, TRUE, TRUE, 0);	
	ss = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(dialog2.combo01));
	gtk_label_set_text(GTK_LABEL(dialog2.label09), ss); 

	dialog2.hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(dialog2.vbox), dialog2.hbox, TRUE, TRUE, 0);
	dialog2.button1 = gtk_button_new_with_label("Update");
	gtk_box_pack_start(GTK_BOX(dialog2.hbox), dialog2.button1, TRUE, TRUE, 0);
	g_signal_connect(dialog2.button1, "clicked", G_CALLBACK(cb_button_clicked1), &dialog2);

	// gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog2.chkbtn), TRUE);


	button1 = gtk_button_new_with_label("Execute");
	gtk_box_pack_start(GTK_BOX(box), button1, TRUE, TRUE, 0);
	g_signal_connect(button1, "clicked", G_CALLBACK(cb_button_clicked_execute), NULL);

	button2 = gtk_button_new_with_label("Quit");
	gtk_box_pack_start(GTK_BOX(box), button2, TRUE, TRUE, 0);
	g_signal_connect(button2, "clicked", G_CALLBACK(cb_button_clicked_quit), NULL);


	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	gtk_widget_show_all(window);
	gtk_main();

	if (NULL == (fp = fopen("chirp_drive.prm", "w"))) {
		fprintf(stderr, "Cannor open chirp_drive.prm\n");
		exit(2);
	}
	fprintf(fp, "%f\tStart Frequency[Hz]\n", dialog.fstart);
	fprintf(fp, "%f\tStop Frequency[Hz]\n", dialog.fstop);
	fprintf(fp, "%f\tPulse Duration[sec]\n", dialog.pduration);
	fprintf(fp, "%d\tChirp Type\n", dialog.type);
	fclose(fp);
	if (NULL == (fp = fopen("chirp_aux.prm", "w"))) {
		fprintf(stderr, "Cannor open chirp_drive.prm\n");
		exit(2);
	}
	fprintf(fp, "%f\tStart Frequency[Hz]\n", dialog2.fstart);
	fprintf(fp, "%f\tStop Frequency[Hz]\n", dialog2.fstop);
	fprintf(fp, "%f\tPulse Duration[sec]\n", dialog2.pduration);
	fprintf(fp, "%d\tChirp Type\n", dialog2.type);
	fclose(fp);

	if (NULL == (fp = fopen("pulse_drive.dat", "w"))) {
		fprintf(stderr, "Cannor open pulse_drive.dat\n");
		exit(2);
	}

	double t, phi, pk, val, pre_val;
	int pwidth = 2;
	int DLEN = (int)(dialog.pduration * 1000000);
	double ptsd = 0.8;
	if (dialog.type == LIN){
		pk = (dialog.fstart - dialog.fstop) / dialog.pduration;
		pre_val = 0.0;
		for(int i = 0; i < DLEN; i++){
			t = (double)i * dltt;
			phi = 2 * M_PI * (dialog.fstart * t - pk / 2. * t * t);
			val = sin(phi);
			if (val > ptsd && pre_val <= ptsd){
				for (int j = 0; j < pwidth; j++){
					fprintf(fp, "1\n");
				}
				i += pwidth-1;
				t = (double)i * dltt;
				phi = 2 * M_PI * (dialog.fstart * t - pk / 2. * t * t);
				val = sin(phi);
			} else {
				fprintf(fp, "0\n");
			}
			pre_val = val;
		}
	} else if (dialog.type == EXP){
		pk = pow(dialog.fstop/dialog.fstart, 1./(1000000. * dialog.pduration));
		pre_val = 0.0;
		for(int i = 0; i < DLEN; i++){
			t = (double)i;
			phi = 2. * M_PI * dialog.fstart * (pow(pk, t) - 1.) / log(pk) * 0.000001;
			val = sin(phi);
			if (val > ptsd && pre_val <= ptsd){
				for (int j = 0; j < pwidth; j++){
					fprintf(fp, "1\n");
				}
				i += pwidth-1;
			t = (double)i;
			phi = 2. * M_PI * dialog.fstart * (pow(pk, t) - 1.) / log(pk) * 0.000001;
			val = sin(phi);
			} else {
				fprintf(fp, "0\n");
			}
			pre_val = val;
		}
	}
	fclose(fp);

	int pwidth_aux = 4;
	if (NULL == (fp = fopen("pulse_drive_aux.dat", "w"))) {
		fprintf(stderr, "Cannor open pulse_drive_aux.dat\n");
		exit(2);
	}
	DLEN = (int)(dialog2.pduration * 1000000);
	if (dialog2.type == LIN){
		pk = (dialog.fstart - dialog.fstop) / dialog.pduration;
		pre_val = 0.0;
		for(int i = 0; i < DLEN; i++){
			t = (double)i * dltt;
			phi = 2 * M_PI * (dialog.fstart * t - pk / 2. * t * t);
			val = sin(phi);
			if (val > 0.8 && pre_val <= 0.8){
				for (int j = 0; j < pwidth_aux; j++){
					fprintf(fp, "1\n");
				}
				i += pwidth_aux-1;
			t = (double)i * dltt;
			phi = 2 * M_PI * (dialog.fstart * t - pk / 2. * t * t);
			val = sin(phi);
			} else {
				fprintf(fp, "0\n");
			}
			pre_val = val;
		}
	} else if (dialog2.type == EXP){
		// pk = pow(dialog2.fstop/dialog2.fstart, 1./dialog2.pduration);
		pk = pow(dialog.fstop/dialog.fstart, 1./(1000000. * dialog.pduration));
		pre_val = 0.0;
		for(int i = 0; i < DLEN; i++){
			t = (double)i;
			// phi = 2. * M_PI * dialog2.fstart * (pow(pk, t) - 1.0) / log(pk);
			phi = 2. * M_PI * dialog.fstart * (pow(pk, t) - 1.) / log(pk) * 0.000001;
			val = sin(phi);
			if (val > 0.8 && pre_val <= 0.8){
				for (int j = 0; j < pwidth_aux; j++){
					fprintf(fp, "1\n");
				}
				i += pwidth_aux-1;
				t = (double)i;
				// phi = 2. * M_PI * dialog2.fstart * (pow(pk, t) - 1.0) / log(pk);
				phi = 2. * M_PI * dialog.fstart * (pow(pk, t) - 1.) / log(pk) * 0.000001;
				val = sin(phi);
			} else {
				fprintf(fp, "0\n");
			}
			pre_val = val;
		}
	}
	fclose(fp);
}

