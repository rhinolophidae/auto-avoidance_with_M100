#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <string.h>
#include <math.h>

#define BUFLEN 256

typedef struct main_dialog_type {
	GtkWidget *window;
	GtkWidget *button1;
	GtkWidget *button2;
	GtkWidget *button3;

	GtkWidget *label01;
	GtkWidget *label02;
	GtkWidget *label03;
	GtkWidget *label04;
	GtkWidget *label05;
	GtkWidget *label06;
	GtkWidget *label07;

	GtkWidget *combo01;
	GtkWidget *combo02;
	GtkWidget *combo03;
	
	GtkWidget *vbox;
	GtkWidget *hbox;
} MainDialog;

const float mes_tim[32] = {33.33332, 33.33332, 28.57142, 25.0, 22.22222,
					 20.0, 18.18182, 17.54386, 17.24138, 16.94916,
					 16.66666, 16.39344, 16.12904, 15.873, 15.38462,
					 14.28572, 13.33334, 12.5, 11.7647, 11.49426,
					 11.36364, 11.23596, 11.1111, 10.989, 10.86956,
					 10.75268, 10.52632, 10.0, 9.5238, 9.0909, 8.69566, 8.33334};
const int mes_num[32] = {1, 2, 20, 40, 60, 80, 100, 120, 140, 160, 180, 200,
						 220, 240, 260, 280, 300, 320, 340, 360, 380, 400,
						 420, 440, 460, 480, 500, 520, 540, 560, 580, 600};

int adc_rec_setting, mes_tim_setting, mes_num_setting;

static void cb_button_clicked1(GtkWidget *widget, gpointer data){
	const gchar *text;
	char text4[256];

	text = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(((MainDialog*)data)->combo01));
	gtk_label_set_text(GTK_LABEL(((MainDialog*)data)->label03), text);
	adc_rec_setting = gtk_combo_box_get_active(GTK_COMBO_BOX(((MainDialog*)data)->combo01))+3;

	text = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(((MainDialog*)data)->combo02));
	gtk_label_set_text(GTK_LABEL(((MainDialog*)data)->label05), text);
	mes_tim_setting = gtk_combo_box_get_active(GTK_COMBO_BOX(((MainDialog*)data)->combo02))+1;

	text = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(((MainDialog*)data)->combo03));
	gtk_label_set_text(GTK_LABEL(((MainDialog*)data)->label07), text);
	mes_num_setting = gtk_combo_box_get_active(GTK_COMBO_BOX(((MainDialog*)data)->combo03));
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


int main(int argc, char *argv[]){
	FILE *fp;
	char buf[BUFLEN];
	MainDialog dialog;
	char **qq;
	int aa;
	aa = 0;
	char s[64];

	if (NULL == (fp = fopen("sensing_fpga.prm", "r"))){
		fprintf(stderr, "Cannot open sensing_fpga.prm\n");
		exit(2);
	}
	fgets(buf, BUFLEN, fp); adc_rec_setting = atoi(buf);
	fgets(buf, BUFLEN, fp); mes_tim_setting = atoi(buf);
	fgets(buf, BUFLEN, fp); mes_num_setting = atoi(buf);
	if (adc_rec_setting < 3 || adc_rec_setting > 16) {
		adc_rec_setting = 3;
	}
	if (mes_tim_setting < 1 || mes_tim_setting > 31) {
		mes_tim_setting = 1;
	}
	if (mes_num_setting < 0 || mes_num_setting > 31) {
		mes_num_setting = 0;
	}
	fclose(fp);

	gtk_init(&aa, &qq);

	dialog.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_size_request(dialog.window, 600, 400);
	gtk_window_set_title(GTK_WINDOW(dialog.window), "Pulse Compression Method");
	gtk_window_set_position(GTK_WINDOW(dialog.window), GTK_WIN_POS_CENTER);
	gtk_container_set_border_width(GTK_CONTAINER(dialog.window), 10);

	dialog.vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_container_add(GTK_CONTAINER(dialog.window), dialog.vbox);

	dialog.hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(dialog.vbox), dialog.hbox, TRUE, TRUE, 0);
	dialog.label01 = gtk_label_new("Enter parameters");
	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.label01, TRUE, TRUE, 0);	

	dialog.hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(dialog.vbox), dialog.hbox, TRUE, TRUE, 0);
	dialog.label02 = gtk_label_new("ADC receive num");
	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.label02, TRUE, TRUE, 0);	
	sprintf(s, "%d", 1 << adc_rec_setting);
	dialog.label03 = gtk_label_new(s);
	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.label03, TRUE, TRUE, 0);	
	dialog.combo01 = gtk_combo_box_text_new();
	for (int i=3; i < 17; i++){
		sprintf(s, "%d", 1<<i);
		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(dialog.combo01), NULL, s);
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(dialog.combo01), adc_rec_setting-3);
	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.combo01, TRUE, TRUE, 0);	

	dialog.hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(dialog.vbox), dialog.hbox, TRUE, TRUE, 0);
	dialog.label04 = gtk_label_new("measurement_time [ms]");
	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.label04, TRUE, TRUE, 0);	
	sprintf(s, "%8.5f", mes_tim[mes_tim_setting]);
	dialog.label05 = gtk_label_new(s);
	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.label05, TRUE, TRUE, 0);	
	dialog.combo02 = gtk_combo_box_text_new();
	for (int i=1; i < 32; i++){
		sprintf(s, "%8.5f", mes_tim[i]);
		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(dialog.combo02), NULL, s);
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(dialog.combo02), mes_tim_setting-1);
	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.combo02, TRUE, TRUE, 0);	


	dialog.hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(dialog.vbox), dialog.hbox, TRUE, TRUE, 0);
	dialog.label06 = gtk_label_new("Measurement num");
	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.label06, TRUE, TRUE, 0);	
	sprintf(s, "%d", mes_num[mes_num_setting]);
	dialog.label07 = gtk_label_new(s);
	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.label07, TRUE, TRUE, 0);	
	dialog.combo03 = gtk_combo_box_text_new();
	for (int i=0; i < 32; i++){
		sprintf(s, "%d", mes_num[i]);
		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(dialog.combo03), NULL, s);
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(dialog.combo03), mes_num_setting);
	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.combo03, TRUE, TRUE, 0);	

	dialog.hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(dialog.vbox), dialog.hbox, TRUE, TRUE, 0);
	dialog.button1 = gtk_button_new_with_label("Update");
	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.button1, TRUE, TRUE, 0);
	g_signal_connect(dialog.button1, "clicked", G_CALLBACK(cb_button_clicked1), &dialog);

	dialog.hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(dialog.vbox), dialog.hbox, TRUE, TRUE, 0);
	dialog.button2 = gtk_button_new_with_label("Execute");
	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.button2, TRUE, TRUE, 0);
	g_signal_connect(dialog.button2, "clicked", G_CALLBACK(cb_button_clicked_execute), NULL);

	dialog.hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(dialog.vbox), dialog.hbox, TRUE, TRUE, 0);
	dialog.button3 = gtk_button_new_with_label("Quit");
	gtk_box_pack_start(GTK_BOX(dialog.hbox), dialog.button3, TRUE, TRUE, 0);
	g_signal_connect(dialog.button3, "clicked", G_CALLBACK(cb_button_clicked_quit), NULL);

	g_signal_connect(dialog.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	gtk_widget_show_all(dialog.window);
	gtk_main();

	if (NULL == (fp = fopen("sensing_fpga.prm", "w"))) {
		fprintf(stderr, "Cannor open sensing_fpga.prm\n");
		exit(2);
	}
	fprintf(fp, "%d\tADC receive num setting\n", adc_rec_setting);
	fprintf(fp, "%d\tmeasurement time setting\n", mes_tim_setting);
	fprintf(fp, "%d\tmeasurement num setting\n", mes_num_setting);
	fprintf(fp, "\n");
	fprintf(fp, "%d\tADC receive num\n", 1<<adc_rec_setting);
	fprintf(fp, "%2.5f\tmeasurement time[ms]\n", mes_tim[mes_tim_setting]);
	fprintf(fp, "%d\tmeasurement num\n", mes_num[mes_num_setting]);
	fclose(fp);
}

