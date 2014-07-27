//twoDhist_widget.cpp - a framed widget to display a TH2F using QCustomPlot.

//Author: Dan Saunders
//Date created: 15/01/13
//Last modified: 20/01/13


#ifndef __TWODHIST_WIDGET_INCLUDED__
#define __TWODHIST_WIDGET_INCLUDED__

#include <iostream>
#include <sstream>
#include <QWidget>

#include "TH2F.h"
#include "TH1F.h"
#include "TCanvas.h"
#include "TImage.h"
#include "qcustomplot.h"
#include <iomanip>

#include <QLabel>
#include <QCheckBox>
#include <QGridLayout>


class twoDhist_widget : public QWidget {
    Q_OBJECT
public:

    //The constructor takes a TH2F, some identifying integer, and addresses
    //to objects out of global scope that change when a plot is selected (clicked).
    twoDhist_widget(TH2F*, int, QCustomPlot* &, int &, QWidget* &, twoDhist_widget* &, QLabel*,
        QLabel*, QLabel*, QLabel*, QCheckBox*, QCheckBox*, QCheckBox*, QLabel*, QLabel*, QLabel*,
        TH1F *);


    QCustomPlot **      _sel_Qh;
    QWidget **          _sel_widg;
    twoDhist_widget **  _sel_hist_widg;
    int *               _sel_ichip;
    QLabel *            _sel_x_lab;
    QLabel *            _sel_y_lab;
    QLabel *            _sel_z_lab;
    QLabel *            _plot_ops_statsbox;
    QCPColorMap *       _colormap;
    QLabel *            _clicked_pos_lab;

    QCheckBox*          _logx_chbox;
    QCheckBox*          _logy_chbox;
    QCheckBox*          _logz_chbox;
    TH1F*               _fit_results;

    QWidget*            _plot_frame;
    TH2F*               _h;
    QCustomPlot*        _Qh;
    int                 _ichip;
    double              _nonzero_xmin, _nonzero_xmax, _nonzero_ymin, _nonzero_ymax;
    bool                _logged_x;
    bool                _logged_y;
    bool                _logged_z;
    QLabel *            _plot_ops_name_box;
    QLabel *            _fit_results_box;
    QString             _x_label;
    QString             _y_label;
    QString             _z_label;



    //Member functions ________________________________________________________
    void                update_plotops_statsbox();
    void                TH2F_to_QcustomPlot();
    void                add_qh_functionality();
    void                add_stats_box();
    void                get_nonzero_range(double &, double &, double &, double &);
    void                make_selected();
    void                fill_fit_results();
    void                toggle_axes_labels();
    void                show_axes_labels();
    void                toggle_ref_plot();


private slots:
    void                mousePress(QMouseEvent*);
    void                mouseWheel();
    void                selectionChanged();
};

#endif
