# file: plot_hidra_summary.py
import os
import sys
from array import array

import numpy as np
import pandas as pd
import ROOT


ROOT.gROOT.SetBatch(True)
ROOT.gStyle.SetOptStat(0)

#scaling_s = 1.07
#scaling_c = 1.05

#scaling_s = 1.16
#scaling_c = 1.255

scaling_s = 1.14
scaling_c = 1.27

#myOutDir = "plots/Attenuation7m_2phe"
myOutDir = "plots/AttenuationTB24"

#extraTex2 = "Fibre attenuation set to 10m" 
extraTex2 = "Fibre attenuation tuned to TB24 data: 3.7 m (Sci), 3.9 m (Cer)" 

extraTex1 = "TB25 HG data used for tuning channel noise"
#extraTex1 = "No noise"

extraTex5 = "Correction for multiple photons on the same SiPM cell: ON"
#extraTex5 = " "

extraTex4 = "Poissonian probability to lose phe per channel: #mu = 3 (Sci), 3 (Cer)"
#extraTex4 = " "

def load_summary(csv_path: str) -> pd.DataFrame:
    data = pd.read_csv(csv_path)
    data = data.sort_values("truth_energy").reset_index(drop=True)
    return data


def safe_ratio(num: np.ndarray, den: np.ndarray) -> np.ndarray:
    out = np.full_like(num, np.nan, dtype=float)
    mask = np.isfinite(num) & np.isfinite(den) & (den != 0.0)
    out[mask] = num[mask] / den[mask]
    return out


def safe_ratio_error(
    num: np.ndarray,
    num_err: np.ndarray,
    den: np.ndarray,
    den_err: np.ndarray,
) -> np.ndarray:
    out = np.full_like(num, np.nan, dtype=float)
    mask = (
        np.isfinite(num)
        & np.isfinite(num_err)
        & np.isfinite(den)
        & np.isfinite(den_err)
        & (num != 0.0)
        & (den != 0.0)
    )
    ratio = np.full_like(num, np.nan, dtype=float)
    ratio[mask] = num[mask] / den[mask]
    rel = np.full_like(num, np.nan, dtype=float)
    rel[mask] = np.sqrt((num_err[mask] / num[mask]) ** 2 + (den_err[mask] / den[mask]) ** 2)
    out[mask] = ratio[mask] * rel[mask]
    return out


def to_root_arrays(x, y, ex, ey):
    return (
        array("d", [float(v) for v in x]),
        array("d", [float(v) for v in y]),
        array("d", [float(v) for v in ex]),
        array("d", [float(v) for v in ey]),
    )


def make_graph(
    x: np.ndarray,
    y: np.ndarray,
    ey: np.ndarray,
    color: int,
    marker: int,
    name: str,
    line_style: int = 1,
):
    ex = np.zeros_like(x, dtype=float)
    mask = np.isfinite(x) & np.isfinite(y) & np.isfinite(ey)
    x_use = x[mask]
    y_use = y[mask]
    ex_use = ex[mask]
    ey_use = ey[mask]

    if len(x_use) == 0:
        return None

    x_arr, y_arr, ex_arr, ey_arr = to_root_arrays(x_use, y_use, ex_use, ey_use)
    graph = ROOT.TGraphErrors(len(x_use), x_arr, y_arr, ex_arr, ey_arr)
    graph.SetName(name)
    graph.SetMarkerColor(color)
    graph.SetLineColor(color)
    graph.SetMarkerStyle(marker)
    graph.SetMarkerSize(1.2)
    graph.SetLineWidth(2)
    graph.SetLineStyle(line_style)
    return graph


def fit_resolution_graph(graph: ROOT.TGraphErrors, color: int, fit_name: str):
    if graph is None or graph.GetN() < 2:
        return None, None

    fit = ROOT.TF1(fit_name, "pol1", 0.0, 1.0)
    fit.SetLineColor(color)
    fit.SetLineWidth(2)
    result = graph.Fit(fit, "QS")
    if int(result) != 0:
        return fit, None
    return fit, result


def draw_header(x_left: float, y_top: float, subtitle: str):
    tex0 = ROOT.TLatex(x_left, y_top, "HidraSim")
    tex0.SetNDC()
    tex0.SetTextFont(72)
    tex0.SetTextSize(0.042)
    tex0.Draw()

    tex1 = ROOT.TLatex(x_left + 0.16, y_top, "Preliminary")
    tex1.SetNDC()
    tex1.SetTextFont(42)
    tex1.SetTextSize(0.042)
    tex1.Draw()

    tex2 = ROOT.TLatex(x_left, y_top - 0.055, subtitle)
    tex2.SetNDC()
    tex2.SetTextFont(42)
    tex2.SetTextSize(0.032)
    tex2.Draw()


def setup_canvas(name: str, width: int = 800, height: int = 650):
    canvas = ROOT.TCanvas(name, name, width, height)
    canvas.SetFillColor(0)
    canvas.SetBorderMode(0)
    canvas.SetBorderSize(2)
    canvas.SetTickx(1)
    canvas.SetTicky(1)
    canvas.SetLeftMargin(0.13)
    canvas.SetRightMargin(0.05)
    canvas.SetTopMargin(0.09)
    canvas.SetBottomMargin(0.11)
    canvas.SetFrameBorderMode(0)
    return canvas


def plot_energy_resolution(data: pd.DataFrame, out_dir: str):
    energy = data["truth_energy"].to_numpy(dtype=float)
    inv_sqrt_e = 1.0 / np.sqrt(energy)

    s_res = safe_ratio(
        data["s_fit_rms"].to_numpy(dtype=float),
        data["s_fit_mean"].to_numpy(dtype=float),
    )
    s_res_err = safe_ratio_error(
        data["s_fit_rms"].to_numpy(dtype=float),
        data["s_fit_rms_err"].to_numpy(dtype=float),
        data["s_fit_mean"].to_numpy(dtype=float),
        data["s_fit_mean_err"].to_numpy(dtype=float),
    )

    c_res = safe_ratio(
        data["c_fit_rms"].to_numpy(dtype=float),
        data["c_fit_mean"].to_numpy(dtype=float),
    )
    c_res_err = safe_ratio_error(
        data["c_fit_rms"].to_numpy(dtype=float),
        data["c_fit_rms_err"].to_numpy(dtype=float),
        data["c_fit_mean"].to_numpy(dtype=float),
        data["c_fit_mean_err"].to_numpy(dtype=float),
    )

    comb_res = safe_ratio(
        data["comb_fit_rms"].to_numpy(dtype=float),
        data["comb_fit_mean"].to_numpy(dtype=float),
    )
    comb_res_err = safe_ratio_error(
        data["comb_fit_rms"].to_numpy(dtype=float),
        data["comb_fit_rms_err"].to_numpy(dtype=float),
        data["comb_fit_mean"].to_numpy(dtype=float),
        data["comb_fit_mean_err"].to_numpy(dtype=float),
    )

    canvas = setup_canvas("c_resolution")
    mg = ROOT.TMultiGraph()
    mg.SetTitle("Energy resolution;1/#sqrt{E_{beam}} [GeV^{-1/2}];#sigma(E)/E")

    graph_comb = make_graph(inv_sqrt_e, comb_res, comb_res_err, ROOT.kGreen + 2, 20, "graph_comb")
    graph_s = make_graph(inv_sqrt_e, s_res, s_res_err, ROOT.kRed + 1, 21, "graph_s")
    graph_c = make_graph(inv_sqrt_e, c_res, c_res_err, ROOT.kBlue + 1, 22, "graph_c")

    for graph in (graph_comb, graph_s, graph_c):
        if graph is not None:
            mg.Add(graph, "PE")

    mg.Draw("A")
    mg.GetXaxis().SetTitleOffset(1.15)
    mg.GetYaxis().SetTitleOffset(1.35)

    fit_comb, _ = fit_resolution_graph(graph_comb, ROOT.kGreen + 2, "fit_comb")
    fit_s, _ = fit_resolution_graph(graph_s, ROOT.kRed + 1, "fit_s")
    fit_c, _ = fit_resolution_graph(graph_c, ROOT.kBlue + 1, "fit_c")


    alignLeft = 0.18
    alignTop = 0.85
    alignRight = 0.925
    tex0 = ROOT.TLatex(alignLeft, alignTop, "HidraSim")
    tex0.SetNDC()
    tex0.SetTextFont(72);
    tex0.SetTextSize(0.042);
    tex0.SetLineWidth(2);
    tex0.Draw("same");	
    #tex1 = ROOT.TLatex(alignLeft+0.16,alignTop,"Preliminary ");
    tex1 = ROOT.TLatex(alignLeft+0.16,alignTop,"Work in progress");
    tex1.SetNDC();
    tex1.SetTextFont(42);
    tex1.SetTextSize(0.042);
    tex1.SetLineWidth(2);
    tex1.Draw("same");	

    legend = ROOT.TLegend(0.16, 0.57, 0.72, 0.82)
    legend.SetBorderSize(0)
    legend.SetFillStyle(0)
    legend.SetTextFont(42)
    legend.SetTextSize(0.03)

    if fit_comb is not None:
        legend.AddEntry(
            fit_comb,
            "Combined: #sigma/E = %.2f%%/#sqrt{E} %+ .2f%%"
            % (100.0 * fit_comb.GetParameter(1), 100.0 * fit_comb.GetParameter(0)),
            "l",
        )
    if fit_s is not None:
        legend.AddEntry(
            fit_s,
            "S: #sigma/E = %.2f%%/#sqrt{E} %+ .2f%%"
            % (100.0 * fit_s.GetParameter(1), 100.0 * fit_s.GetParameter(0)),
            "l",
        )
    if fit_c is not None:
        legend.AddEntry(
            fit_c,
            "C: #sigma/E = %.2f%%/#sqrt{E} %+ .2f%%"
            % (100.0 * fit_c.GetParameter(1), 100.0 * fit_c.GetParameter(0)),
            "l",
        )

    legend.Draw()
    draw_header(0.16, 0.84, "Energy resolution from Gaussian fits")
    canvas.SaveAs(os.path.join(out_dir, "energy_resolution.pdf"))


def plot_energy_linearity(data: pd.DataFrame, out_dir: str):
    energy = data["truth_energy"].to_numpy(dtype=float)

    s_mean = data["s_fit_mean"].to_numpy(dtype=float)
    s_mean_err = data["s_fit_mean_err"].to_numpy(dtype=float)
    c_mean = data["c_fit_mean"].to_numpy(dtype=float)
    c_mean_err = data["c_fit_mean_err"].to_numpy(dtype=float)
    comb_mean = data["comb_fit_mean"].to_numpy(dtype=float)
    comb_mean_err = data["comb_fit_mean_err"].to_numpy(dtype=float)

    s_fers_on_mean = data["s_fit_fers_on_mean"].to_numpy(dtype=float)
    s_fers_on_mean_err = data["s_fit_fers_on_mean_err"].to_numpy(dtype=float)
    c_fers_on_mean = data["c_fit_fers_on_mean"].to_numpy(dtype=float)
    c_fers_on_mean_err = data["c_fit_fers_on_mean_err"].to_numpy(dtype=float)

    #s_lin = safe_ratio(s_mean - energy, energy)
    s_lin = safe_ratio(s_mean, energy)
    s_lin_err = safe_ratio(s_mean_err, energy)

    #c_lin = safe_ratio(c_mean - energy, energy)
    c_lin = safe_ratio(c_mean, energy)
    c_lin_err = safe_ratio(c_mean_err, energy)

    #comb_lin = safe_ratio(comb_mean - energy, energy)
    comb_lin = safe_ratio(comb_mean, energy)
    comb_lin_err = safe_ratio(comb_mean_err, energy)

    #s_fers_on_lin = safe_ratio(s_fers_on_mean - energy, energy)
    s_fers_on_lin = safe_ratio(s_fers_on_mean*scaling_s, energy)
    s_fers_on_lin_err = safe_ratio(s_fers_on_mean_err, energy)

    #c_fers_on_lin = safe_ratio(c_fers_on_mean - energy, energy)
    c_fers_on_lin = safe_ratio(c_fers_on_mean*scaling_c, energy)
    c_fers_on_lin_err = safe_ratio(c_fers_on_mean_err, energy)

    canvas = setup_canvas("c_linearity")
    mg = ROOT.TMultiGraph()
    #mg.SetTitle("Energy linearity;E_{beam} [GeV];(E_{fit}-E_{beam})/E_{beam}")
    mg.SetTitle("Energy linearity;E_{beam} [GeV];E_{fit} / E_{beam}")

    graph_comb = make_graph(energy, comb_lin, comb_lin_err, ROOT.kGreen + 2, 20, "lin_comb")
    graph_s = make_graph(energy, s_lin, s_lin_err, ROOT.kRed + 1, 21, "lin_s")
    graph_c = make_graph(energy, c_lin, c_lin_err, ROOT.kBlue + 1, 22, "lin_c")
    graph_s_fers_on = make_graph(
        energy,
        s_fers_on_lin,
        s_fers_on_lin_err,
        ROOT.kRed + 1,
        25,
        "lin_s_fers_on",
        line_style=1,
    )
    graph_c_fers_on = make_graph(
        energy,
        c_fers_on_lin,
        c_fers_on_lin_err,
        ROOT.kBlue + 1,
        26,
        "lin_c_fers_on",
        line_style=1,
    )

    #for graph in (graph_comb, graph_s, graph_c, graph_s_fers_on, graph_c_fers_on):
    for graph in (graph_comb, graph_s, graph_c):
        if graph is not None:
            mg.Add(graph, "PE")

    mg.Add(graph_s_fers_on, "PEL")
    mg.Add(graph_c_fers_on, "PEL")

    mg.Draw("A")
    mg.GetXaxis().SetTitleOffset(1.07)
    mg.GetYaxis().SetTitleOffset(1.2)
    #mg.SetMinimum(-0.40)
    #mg.SetMaximum(0.08)
    
    mg.SetMinimum(+0.9)
    mg.SetMaximum(+1.14)

    xmin = float(np.nanmin(energy))
    xmax = float(np.nanmax(energy))
    #for y_val, color in ((0.0, ROOT.kGreen + 3), (0.01, ROOT.kGray + 2), (-0.01, ROOT.kGray + 2)):
    #    line = ROOT.TLine(xmin, y_val, xmax, y_val)
    #    line.SetLineStyle(2)
    #    line.SetLineColor(color)
    #    line.Draw()


    line1=ROOT.TLine(mg.GetXaxis().GetXmin(), 1.01, mg.GetXaxis().GetXmax(), 1.01)
    line2=ROOT.TLine(mg.GetXaxis().GetXmin(), 0.99, mg.GetXaxis().GetXmax(), 0.99)
    line3=ROOT.TLine(mg.GetXaxis().GetXmin(), 1.0, mg.GetXaxis().GetXmax(), 1.0)
    line1.SetLineStyle(2)
    line2.SetLineStyle(2)
    line3.SetLineStyle(2)
    line3.SetLineColor(ROOT.kGreen+3)

    line1.Draw("same")
    line2.Draw("same")
    line3.Draw("same")
    line2.Draw("same")
    line3.Draw("same")

    alignLeft = 0.18
    alignTop = 0.85
    alignRight = 0.925
    tex0 = ROOT.TLatex(alignLeft, alignTop, "HidraSim")
    tex0.SetNDC()
    tex0.SetTextFont(72);
    tex0.SetTextSize(0.042);
    tex0.SetLineWidth(2);
    tex0.Draw("same");	
    #tex1 = ROOT.TLatex(alignLeft+0.16,alignTop,"Preliminary ");
    tex1 = ROOT.TLatex(alignLeft+0.16,alignTop,"Work in progress");
    tex1.SetNDC();
    tex1.SetTextFont(42);
    tex1.SetTextSize(0.042);
    tex1.SetLineWidth(2);
    tex1.Draw("same");	
    tex2 = ROOT.TLatex(alignLeft,alignTop-0.12, extraTex2)
    tex2.SetNDC();
    tex2.SetTextFont(42);
    tex2.SetTextSize(0.03);
    tex2.SetLineWidth(2);
    tex2.Draw("same");	
    tex3 = ROOT.TLatex(alignLeft,alignTop-0.08, extraTex1)
    tex3.SetNDC();
    tex3.SetTextFont(42);
    tex3.SetTextSize(0.03);
    tex3.SetLineWidth(2);
    tex3.Draw("same");	
    tex4 = ROOT.TLatex(alignLeft,alignTop-0.04, extraTex4)
    tex4.SetNDC();
    tex4.SetTextFont(42);
    tex4.SetTextSize(0.03);
    tex4.SetLineWidth(2);
    tex4.Draw("same");	
    tex5 = ROOT.TLatex(alignLeft,alignTop-0.16, extraTex5)
    tex5.SetNDC();
    tex5.SetTextFont(42);
    tex5.SetTextSize(0.03);
    tex5.SetLineWidth(2);
    tex5.Draw("same");	


    legend = ROOT.TLegend(0.450, 0.17, 0.83, 0.37)
    legend.SetBorderSize(0)
    legend.SetFillStyle(0)
    legend.SetTextFont(42)
    legend.SetTextSize(0.03)
    legend.SetHeader("Fers thresholds tuned to TB data")  # Set the header for the legend
    if graph_comb is not None:
        legend.AddEntry(graph_comb, "Combined - all fibres", "pe")
    if graph_s is not None:
        legend.AddEntry(graph_s, "S - all fibres", "pe")
    if graph_c is not None:
        legend.AddEntry(graph_c, "C - all fibres", "pe")
    if graph_s_fers_on is not None:
        #legend.AddEntry(graph_s_fers_on, "S - FERS On only, Thr: >= 1 phe #times 1.1", "pe")
        legend.AddEntry(graph_s_fers_on, "S - FERS On only #times " + str(scaling_s), "ple")
    if graph_c_fers_on is not None:
        #legend.AddEntry(graph_c_fers_on, "C - FERS On only, Thr: >= 1 phe #times 1.1", "pe")
        legend.AddEntry(graph_c_fers_on, "C - FERS On only #times " + str(scaling_c), "ple")
    legend.Draw()

    draw_header(0.16, 0.84, "Linearity from fitted energy means")
    canvas.SaveAs(os.path.join(out_dir, "energy_linearity.pdf"))


def plot_spatial_resolution(data: pd.DataFrame, out_dir: str):
    energy = data["truth_energy"].to_numpy(dtype=float)

    configs = [
        (
            "spatial_resolution_x.pdf",
            "Spatial resolution X;E_{beam} [GeV];Residual RMS [mm]",
            data["res_sci_x_rms"].to_numpy(dtype=float),
            data["res_sci_x_rms_err"].to_numpy(dtype=float),
            data["res_cer_x_rms"].to_numpy(dtype=float),
            data["res_cer_x_rms_err"].to_numpy(dtype=float),
            "Residual RMS along X",
        ),
        (
            "spatial_resolution_y.pdf",
            "Spatial resolution Y;E_{beam} [GeV];Residual RMS [mm]",
            data["res_sci_y_rms"].to_numpy(dtype=float),
            data["res_sci_y_rms_err"].to_numpy(dtype=float),
            data["res_cer_y_rms"].to_numpy(dtype=float),
            data["res_cer_y_rms_err"].to_numpy(dtype=float),
            "Residual RMS along Y",
        ),
    ]

    for file_name, title, sci_rms, sci_rms_err, cer_rms, cer_rms_err, subtitle in configs:
        canvas = setup_canvas(file_name.replace(".pdf", ""))
        mg = ROOT.TMultiGraph()
        mg.SetTitle(title)

        graph_sci = make_graph(energy, sci_rms, sci_rms_err, ROOT.kRed + 1, 21, f"sci_{file_name}")
        graph_cer = make_graph(energy, cer_rms, cer_rms_err, ROOT.kBlue + 1, 22, f"cer_{file_name}")

        if graph_sci is not None:
            mg.Add(graph_sci, "PE")
        if graph_cer is not None:
            mg.Add(graph_cer, "PE")

        mg.Draw("A")
        mg.GetXaxis().SetTitleOffset(1.15)
        mg.GetYaxis().SetTitleOffset(1.35)

        legend = ROOT.TLegend(0.16, 0.70, 0.34, 0.82)
        legend.SetBorderSize(0)
        legend.SetTextFont(42)
        legend.SetTextSize(0.03)
        if graph_sci is not None:
            legend.AddEntry(graph_sci, "Sci", "pe")
        if graph_cer is not None:
            legend.AddEntry(graph_cer, "Cer", "pe")
        legend.Draw()

        draw_header(0.16, 0.84, subtitle)
        canvas.SaveAs(os.path.join(out_dir, file_name))


def main():
    csv_path = sys.argv[1] if len(sys.argv) > 1 else "hidra_summary.csv"
    out_dir = sys.argv[2] if len(sys.argv) > 2 else myOutDir

    os.makedirs(out_dir, exist_ok=True)

    data = load_summary(csv_path)
    plot_energy_resolution(data, out_dir)
    plot_energy_linearity(data, out_dir)
    plot_spatial_resolution(data, out_dir)

    print(f"Read: {csv_path}")
    print(f"Wrote plots to: {out_dir}")


if __name__ == "__main__":
    main()