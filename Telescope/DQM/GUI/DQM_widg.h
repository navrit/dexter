//mainwindow.h - the opening window of the DQM. This window needs to be able
//to start the DQM (and set its inital options), and display all the
//resulting plots. These need to be replaceable (on pressing restart), and
//updatable.

//Author: Dan Saunders
//Date created: 07/01/13
//Last modified: 20/01/13


#ifndef __MAINWINDOW_H__
#define __MAINWINDOW_H__

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cstdio>
#include <iomanip>
#include <string>
#include <sstream>

#include "TH1F.h"
#include "TFile.h"

#include "../CDQM_options.h"
#include "../headers/CDQM.h"

#include <QMainWindow>
#include "qcustomplot.h"
#include "hist_widget.h"
#include "twoDhist_widget.h"
#include "TASImage.h"
#include "ui_mainwindow.h" //made by qmake.
#include <QTimer>
#include <QFileDialog>
#include <QProgressBar>


namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow{
    Q_OBJECT

public:
    Ui::MainWindow*     ui; //the DQM window.
    CDQM*               _DQM; //instance of the DQM, with switch.
    bool                _DQM_ran;
    CDQM_options*       _ops;
    int                 _nchips; //assumed equal to number of ADC dists.
    TFile*              _tfile; //DQM output file, with switch.
    bool                _fileopen;
    std::string         _ref_file;
    int                 _nsample;
    QDateTime*          _DQMclock;
    QTimer*             _timer;
    bool                _AutoUpdate;
    bool                _liveVsSaved;


    //Pointers to the currently selected plots.
    QCustomPlot*        _sel_Q;
    int                 _sel_ichip;
    QWidget*            _sel_widg;
    hist_widget*        _sel_hist;
    twoDhist_widget*    _sel_twoDhist;

    bool _include_references;

    std::vector<hist_widget*> _ADC_distribution_hists;
    std::vector<hist_widget*> _temporal_differences_hists;
    std::vector<twoDhist_widget*> _hitmap_hists;
    std::vector<twoDhist_widget*> _xcorrel_hists;
    std::vector<twoDhist_widget*> _ycorrel_hists;

    std::vector<hist_widget*> _clust_ADC_distribution_hists;
    std::vector<hist_widget*> _clust_ADC_distribution_hists4;
    std::vector<hist_widget*> _clust_ADC_distribution_hists1;
    std::vector<hist_widget*> _clust_ADC_distribution_hists2;
    std::vector<hist_widget*> _clust_ADC_distribution_hists3;
    std::vector<hist_widget*> _clust_temporal_differences_hists;
    std::vector<twoDhist_widget*> _clust_hitmap_hists;
    std::vector<twoDhist_widget*> _clust_xcorrel_hists;
    std::vector<twoDhist_widget*> _clust_ycorrel_hists;
    std::vector<hist_widget*> _clust_size_dist_hists;
    std::vector<twoDhist_widget*> _clust_samples_hists;

    std::vector<hist_widget*> _XResidualsHists;
    std::vector<hist_widget*> _YResidualsHists;
    std::vector<hist_widget*> _TResidualsHists;



    //Member functions ________________________________________________________
    explicit            MainWindow(int r = 0);
                        ~MainWindow();


    //Methods to fill the tabs.
    void                fill_plot_tabs();
    void                set_ops_tab();
    void                fill_chip_views();
    void                fill_correlations(int, int, bool);
    void                fill_differences(int, int, bool);
    void                fill_hitmaps(int);
    void                fill_hitmapsProj(int);
    void                fill_ADC_Distributions(int);
    void                fill_z_distribution(int);
    void                fill_temporal_differences(int);
    void                fill_clust_samples();
    void                fill_clust_sizes();
    void                fill_residuals(int);
    void                fill_generalTrackPlots();
    void                fill_ResidualsVarsTab();
    void				addChipBox(twoDhist_widget *, int, int);
    void                fill_event_view();



    //Methods to get the various plot widgets.
    QCustomPlot*        get_z_dist_widget(int);
    hist_widget*        get_track_size_widget(int);
    hist_widget*        get_hitmapProj_widget(int, int);
    twoDhist_widget*    get_correl_widget(int, int, int, bool);
    twoDhist_widget*    get_hitmap_widget(int, int);
    hist_widget*        get_ADC_widget(int, int);
    hist_widget*        get_temp_diff_widget(int, int);
    hist_widget*        get_residual_var(int, int);
    hist_widget*        get_diff_widget(int, int, int, bool);
    hist_widget*        get_cluster_size_widget(int);
    twoDhist_widget*    get_cluster_samples_widget(int);
    void                empty_tab(QWidget*);
    void                ops_apply();
    hist_widget*        get_residual(int, int);





private slots:
    //Buttons _________________________________________________________________
    void                on_init_but_clicked();
    void                on_append_but_clicked();
    void                on_rescale_but_clicked();
    void                on_open_TBrowser_but_clicked();
    void                on_exit_but_clicked();
    void                on_load_but_clicked();
    void                on_togglerefBut_clicked();
    void                on_togglelabBut_clicked();
    void                timer_update();


    //Tab actions.
    void                on_pixel_tabs_currentChanged(int index);


    //Checkbox actions.
    void                on_logx_chbox_clicked(bool checked);
    void                on_logy_chbox_clicked(bool checked);
    void                on_logz_chbox_clicked(bool checked);



    void on_pushButton_2_clicked();
    void on_pushButton_clicked();
};


#endif
