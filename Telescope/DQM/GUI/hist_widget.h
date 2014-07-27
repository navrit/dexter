//hist_widget.cpp - a framed widget to display a TH2F using QCustomPlot.

//Author: Dan Saunders
//Date created: 15/01/13
//Last modified: 20/01/13


#ifndef __HIST_WIDGET_INCLUDED__
#define __HIST_WIDGET_INCLUDED__

#include <QWidget>
#include <iostream>
#include <sstream>

#include "TH1F.h"
#include "TGraph.h"
#include "TF1.h"
#include "qcustomplot.h"
#include <iomanip>
#include <QLabel>
#include <QCheckBox>
#include <QGridLayout>


class hist_widget : public QWidget {
    Q_OBJECT
public:

    //The constructor takes a TH2F, some identifying integer, and addresses
    //to objects out of global scope that change when a plot is selected (clicked).
    hist_widget(TH1F*, int, QCustomPlot* &, int &, QWidget* &, hist_widget* &,
          QLabel*, QLabel*, QLabel*, QLabel*, QCheckBox*, QCheckBox*, QCheckBox*,
          QLabel*, QLabel*, QLabel*, TH1F* h_ref = NULL);


    hist_widget(TGraph*, int, QCustomPlot* &, int &, QWidget* &, hist_widget* &,
          QLabel*, QLabel*, QLabel*, QLabel*, QCheckBox*, QCheckBox*, QCheckBox*,
          QLabel*, QLabel*, QLabel*, TH1F* h_ref = NULL);


    QCustomPlot**       _sel_Qh;
    QWidget**           _sel_widg;
    hist_widget**       _sel_hist_widg;
    QLabel *            _sel_x_lab;
    QLabel *            _sel_y_lab;
    QLabel *            _sel_z_lab;
    QLabel *            _clicked_pos_lab;
    int*                _sel_ichip;

    QCheckBox*          _logx_chbox;
    QCheckBox*          _logy_chbox;
    QCheckBox*          _logz_chbox;
    bool                _axes_labels;
    bool                _ref_showing;


    TH1F*               _h;
    TH1F*               _ref_h;
    QWidget*            _plot_frame;
    QCustomPlot*        _Qh;
    QLabel *            _plot_ops_statsbox;
    int                 _ichip;
    bool                _logged_x;
    bool                _logged_y;
    float               _fit_parameters[3];
    QLabel *            _plot_ops_name_box;
    QLabel *            _fit_results_box;
    std::string         _fit_function_name; // One at a time.

    QString             _x_label;
    QString             _y_label;


    //Member functions ________________________________________________________
    void                update_plotops_statsbox();
    void                TH1F_to_QcustomPlot(TH1F *, QCustomPlot *);
    void                TGraph_to_QcustomPlot(TGraph *, QCustomPlot *);
    void                add_qh_functionality();
    void                add_stats_box(TH1F*);
    void                add_mean_line();
    void                make_selected();
    void                fit_landau(QColor);
    void                fit_gaussian(QColor);
    void                toggle_axes_labels();
    void                toggle_ref_plot();
    void                show_axes_labels();


private slots:
    void                mousePress(QMouseEvent*);
    void                mouseWheel();
    void                selectionChanged();
};

#endif
