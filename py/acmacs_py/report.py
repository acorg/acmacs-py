"""
Generates multi-page pdf, aka report

from pathlib import Path
from acmacs_py.report import Report, Cover, TableOfContents, Section, Subsection

with Report(pdf=Path("/tmp/a.pdf"), open_pdf=True) as rp:
    rp.add(Cover(title=r"Information for the WHO Consultation\\ on the Composition of Influenza Vaccines\\ for the Southern Hemisphere 2042%",
                 subtitels=["Teleconference 13", "", "", "", "31 Aidar 2042"],
                 bottom=["Center for Pathogen Evolution", "University of Cambridge, United Kingdom"]
                 ),
           TableOfContents(),
           Section("H1N1"),
           Subsection("Maps")
           )

"""

import subprocess, datetime
from pathlib import Path
from contextlib import contextmanager

LOCAL_TIMEZONE = datetime.datetime.now(datetime.timezone.utc).astimezone().tzinfo # https://stackoverflow.com/questions/2720319/python-figure-out-local-timezone

# ssm report
#
#         modul.geographic_ts(sorted(Path("geo").glob("H1-geographic-*.pdf"))),
#         modul.maps_in_two_columns([


# # latex.T_ColorsBW,
# latex.T_ColorsColors,
# latex.T_ColorCodedBy,
# latex.T_AntigenicMapTable,
# latex.T_WhoccStatisticsTable,
# T_GeographicMapsTable
# latex.T_SignaturePage,

# ======================================================================

class Cover:

    def __init__(self, title: str, subtitels: list[str] = [], bottom: list[str] = [], space_top: str = "130pt", title_font_size: str = "{22}{26}", subtitle_font_size: str="{19}{24}", space_above_bottom: str = "180pt"):
        self.title = title
        self.subtitels = subtitels
        self.bottom = bottom
        self.space_top = space_top
        self.space_above_bottom = space_above_bottom
        self.title_font_size = title_font_size
        self.subtitle_font_size = subtitle_font_size

    def latex(self) -> str:
        return r"""
               \thispagestyle{empty}

               {\quotation
               \vspace*{%(space_top)s}
               {
               \fontsize%(title_font_size)s \selectfont
               \noindent
               \textbf{%(title)s}
               %% \par
               }

               \vspace{90pt}
               {
               \fontsize%(subtitle_font_size)s \selectfont
               %(subtitels)s
               %% \par
               }

               \vspace{%(space_above_bottom)s}
               {
               \large
               %(bottom)s
               %% do not remove two empty lines below


               %% do not remove two empty lines above!
               \newpage
               }
               }
               """ % {
                   "title": self.title,
                   "space_top": self.space_top,
                   "space_above_bottom": self.space_above_bottom,
                   "title_font_size": self.title_font_size,
                   "subtitle_font_size": self.subtitle_font_size,
                   "subtitels": r"\noindent " + "\n\n\\vspace{10pt} \\noindent\n".join(self.subtitels),
                   "bottom": r"\noindent " + "\n\n\\vspace{10pt} \\noindent\n".join(self.bottom),
                   }

# ----------------------------------------------------------------------

class Newpage:

    def latex(self):
        return r"\newpage"

class Section:

    def __init__(self, title: str):
        self.title = title

    def latex(self):
        return r"\newpage \section{%(title)s}" % {"title": self.title}

class Subsection:

    def __init__(self, title: str):
        self.title = title

    def latex(self):
        return r"\subsection{%(title)s}" % {"title": self.title}

class TableOfContents:

    def latex(self):
        return r"\newpage \tableofcontents"

class VSpace:

    def __init__(self, em: float):
        self.em = em

    def latex(self):
        return r"\vspace{%(em)fem" % {"em": self.em}

class Text:

    def __init__(self, text: str, noindent: bool = False):
        self.text = text
        self.noindent = noindent

    def latex(self):
        return (r"\noindent " if self.noindent else "") + self.text

# ----------------------------------------------------------------------

class SquareImagesInTwoColumns:
    "Antigenic maps"


    def __init__(self, pdfs: list, image_scale: str = "9 / 30", tabcolsep: float = 7.0, arraystretch: float = 3.5):
        if not pdfs or (len(pdfs) % 2) != 0:
            raise RuntimeError(f"invalid pdf list provided: {len(pdfs)}: expected even elements: {pdfs}")
        self.pdfs = pdfs
        self.image_scale = image_scale
        self.tabcolsep = tabcolsep
        self.arraystretch = arraystretch
        # self.images_per_page = 6

    def latex(self):
        return "\n".join([self.begin(), *(self.row(self.pdfs[no], self.pdfs[no+1]) for no in range(0, len(self.pdfs), 2)), self.end()])

    def begin(self):
        if self.image_scale is not None:
            return r"\begin{PdfTable2WithSep}{%fpt}{%f}{%s}" % (self.tabcolsep, self.arraystretch, self.image_scale)
        else:
            return r"\begin{PdfTable2}"

    def end(self):
        if self.image_scale is not None:
            return r"\end{PdfTable2WithSep}"
        else:
            return r"\end{PdfTable2}"

    def row(self, pdf1: Path, pdf2: Path):

        def im(image):
            if image:
                if image.exists():
                    return f"\\PdfTable2Image{{{image.resolve()}}}"
                else:
                    return f"{{\\fontsize{{16}}{{20}} \\selectfont \\noindent \\rotatebox{{45}}{{ \\textbf{{ \\textcolor{{red}}{{{image}}} }} }}}}"
            else:
                return r"\hspace{18em}"

        return f"{im(pdf1)} & {im(pdf2)} \\\\"


# ======================================================================

class ReportMaker:

    def __init__(self, pdf: Path):
        self.pdf = pdf
        self.data = []
        self.page_numbering = True
        self.add_time_stamp = True
        self.packages = set()
        self.paper_size = "a4"
        self.orientation = "portreat"
        self.default_packages = [
               "[cm]{fullpage}",
               "{verbatim}",
               "[table]{xcolor}",
               "{tikz}",               # draw filled circles in \ColorCodedByRegion
               "{graphicx} ",          # multiple pdfs per page, pdf
               "[export]{adjustbox}",  # frame in \includegraphics
               "{grffile}",            # to allow .a.pdf in \includegraphics
               "{pdfpages}",           # phylogenetic tree
               "{fancyhdr}",           # keep page numbers in embedded phylogenetic tree
               "{calc}",
               "{hyperref}",           # ToC entries as links
               "{tocloft}",            # \cftsetindents
               "[toc,page]{appendix}", # Appendice
               "{titletoc}",           # ToC entries without numbers
               "[T1]{fontenc}",        # fonts
               "{times}",              # font
            ]

    def add(self, *source):
        for elt in source:
            for obj in (elt if isinstance(elt, list) else [elt]):
                if not hasattr(obj, "latex"):
                    raise RuntimeError(f"cannot add to report (no latex() method defined): {obj}")
                self.data.append(obj)

    def generate_latex(self, filename: Path):
        # latex.T_WholePagePdf,
        # latex.T_SignaturePage,

        with filename.open("w") as fd:
            fd.write(r"""
               \pagestyle{empty}
               \documentclass[%(paper_size)spaper,%(orientation)s,12pt]{article}

               %(default_packages)s
               %(packages)s
               %(commands)s

               \begin{document}
               \rmfamily

               %(page_numbering)s
               %(document)s

               %(time_stamp)s
               \end{document}
            """ % {
                "paper_size": self.paper_size,
                "orientation": self.orientation,
                "page_numbering": self._make_page_numbering(),
                "commands": self._make_commands(),
                "default_packages": self._make_packages(self.default_packages),
                "packages": self._make_packages(self.packages),
                "document": self._make_document(),
                "time_stamp": self._make_time_stamp(),
                })

    def generate(self, open_pdf: bool):
        latex_filename = self.pdf.with_suffix(".tex")
        self.generate_latex(latex_filename)
        subprocess.check_call(["pdflatex", "-interaction=nonstopmode", "-file-line-error", str(latex_filename.resolve())], cwd=self.pdf.parent)
        self._cleanup()
        if open_pdf:
            subprocess.run(["open", self.pdf])

    def _cleanup(self):
        # needed for TableOfContents # self.pdf.with_suffix(".aux").unlink(missing_ok=True)
        self.pdf.with_suffix(".log").unlink(missing_ok=True)
        self.pdf.with_suffix(".tex").unlink(missing_ok=True)

    def _make_document(self):
        return "\n".join(en.latex() for en in self.data)

    def _make_commands(self):
        return sCommands

    def _make_packages(self, source):
        def mk(pk):
            if isinstance(pk, list):
                return f"[{pk[0]}]{{{pk[1]}}}"
            elif pk[0] in ["[", "{"]:
                return pk
            else:
                return f"{{{pk}}}"
        return "\n".join(f"\\usepackage{mk(pack)}" for pack in source)

    def _make_time_stamp(self):
        if self.add_time_stamp:
            return r"\par\vspace*{\fill}\tiny{generated on %s}" % datetime.datetime.now(LOCAL_TIMEZONE).strftime("%Y-%m-%d %H:%M %Z")
        else:
            return ""

    def _make_page_numbering(self):
        if self.page_numbering:
            return ""
        else:
            return r"\pagenumbering{gobble}"

# ======================================================================

@contextmanager
def Report(pdf: Path, open_pdf: bool = True):
    rp = ReportMaker(pdf=pdf)
    yield rp
    rp.generate(open_pdf=open_pdf)

# ----------------------------------------------------------------------

sCommands = r"""
% ----------------------------------------------------------------------
% Blank page (http://tex.stackexchange.com/questions/36880/insert-a-blank-page-after-current-page)
% ----------------------------------------------------------------------
\newcommand\blankpage{%
  \newpage
  \vspace*{100pt}
  \thispagestyle{empty}%
  \newpage}

% ----------------------------------------------------------------------
% remove section numbering
% ----------------------------------------------------------------------

%% http://www.ehow.com/how_8085363_hide-section-numbers-latex.html
\setcounter{secnumdepth}{-1}

% ----------------------------------------------------------------------
% ToC table of contents
% ----------------------------------------------------------------------

%% ToC http://tex.stackexchange.com/questions/163986/format-table-of-contents-with-latex
\titlecontents{section}[0cm]{\bfseries}{\\}{\\}{}
\titlecontents{subsection}[1em]{}{}{}{\titlerule*[5pc]{}\vspace{0.8ex}\thecontentspage}
\contentsmargin{120pt}

% table of content indentation
% http://tex.stackexchange.com/questions/50471/question-about-indent-lengths-in-toc
\cftsetindents{section}{0.5in}{0.5in}

%% http://tex.stackexchange.com/questions/80113/hide-section-numbers-but-keep-numbering
% \renewcommand{\thesection}{}
% \makeatletter
% \def\@seccntformat#1{\csname #1ignore\expandafter\endcsname\csname the#1\endcsname\quad}
% \let\sectionignore\@gobbletwo
% \let\latex@numberline\numberline
% \def\numberline#1{\if\relax#1\relax\else\latex@numberline{#1}\fi}
% \makeatother
% \renewcommand{\thesubsection}{\arabic{subsection}}

% ----------------------------------------------------------------------
% WholePagePdf
% ----------------------------------------------------------------------
\newenvironment{WholePagePdfEnv}{
   \noindent
   \begin{center}
}{\end{center}\par}
% \newcommand{\WholePagePdf}[1]{\begin{WholePagePdfEnv}\pagestyle{empty} \includepdf[pages=-,pagecommand={\thispagestyle{fancy}}]{#1}\end{WholePagePdfEnv}}
\newcommand{\WholePagePdf}[1]{\begin{WholePagePdfEnv}\pagestyle{empty} \includepdf[pages={1}]{#1}\end{WholePagePdfEnv}}
\newcommand{\WholePagePdfFit}[1]{\begin{WholePagePdfEnv}\includegraphics[page=1,scale=0.9]{#1}\end{WholePagePdfEnv}}
\newcommand{\WholePagePdfTwoToc}[3]{
  \begin{WholePagePdfEnv}
    \includepdf[pages=-,pagecommand={\pagestyle{fancy}}]{#1}
    \addcontentsline{toc}{subsection}{#2}
    \includepdf[pages=-,pagecommand={\pagestyle{fancy}}]{#3}
  \end{WholePagePdfEnv}}


% ----------------------------------------------------------------------
% Table with antigenic maps
% ----------------------------------------------------------------------
\def \PdfTable2ImageSizeSize {(\textheight-20pt) * 9 / 30} % size of an embedded antigenic map
\def \PdfTable2ImageSizeSmallSize {(\textheight-20pt) * 17 / 60} % size of an embedded antigenic map

\newenvironment{PdfTable2}{
  \setlength{\tabcolsep}{7pt}
  \renewcommand{\arraystretch}{3.5}
  \newcommand{\PdfTable2Image}[1]{\includegraphics[width=\PdfTable2ImageSizeSize,frame]{##1}}
  \newcommand{\PdfTable2ImageSmall}[1]{\includegraphics[width=\PdfTable2ImageSizeSmallSize,frame]{##1}}
  \begin{center}
    \begin{tabular}{c c}
}{\end{tabular}\end{center}\par}

\newenvironment{PdfTable2WithSep}[3]{
  \setlength{\tabcolsep}{#1}
  \renewcommand{\arraystretch}{#2}
  \newcommand{\PdfTable2Image}[1]{\includegraphics[width={(\textheight-20pt) * {#3}},frame]{##1}}
  \begin{center}
    \begin{tabular}{c c}
}{\end{tabular}\end{center}\par}
"""

# ----------------------------------------------------------------------
