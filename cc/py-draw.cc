#include "acmacs-base/color-distinct.hh"
#include "acmacs-draw/surface-cairo.hh"
#include "acmacs-py/py.hh"

// ----------------------------------------------------------------------

void acmacs_py::draw(py::module_& mdl)
{
    using namespace pybind11::literals;
    using namespace acmacs::surface;

    using Size = acmacs::Size;
    using TextStyle = acmacs::TextStyle;

    // m.doc() = "Acmacs draw plugin";

    // ----------------------------------------------------------------------
    // Color
    // ----------------------------------------------------------------------

    const auto color_to_string = [](const Color& color) { return fmt::format("{}", color); };

    py::class_<Color>(mdl, "Color")                                                                  //
        .def(py::init<std::string>(), "color"_a = "black")                                           //
        .def("__str__", color_to_string)                                                             //
        .def("__eq__", [](const Color& color, std::string_view rhs) { return color == Color{rhs}; }) //
        .def("__eq__", [](const Color& color, const Color& rhs) { return color == rhs; })            //
        .def("__ne__", [](const Color& color, std::string_view rhs) { return color != Color{rhs}; }) //
        .def("__ne__", [](const Color& color, const Color& rhs) { return color != rhs; })            //
        .def("to_string", color_to_string)                                                           //
        .def("to_hex_string", color_to_string)                                                       //
        // .def("light", &Color::light)
        ;

    mdl.def("distinct_colors", &acmacs::color::distinct_s);

    // ----------------------------------------------------------------------
    // Surface
    // ----------------------------------------------------------------------

    py::class_<acmacs::surface::Surface>(mdl, "Surface")
        .def(
            "subsurface_s",
            [](Surface& aSurface, double x, double y, double width, double sub_width, double sub_height, bool clip) -> Surface& {
                return aSurface.subsurface({x, y}, Scaled{width}, Size{sub_width, sub_height}, clip);
            },
            "origin_x"_a, "origin_y"_a, "width_in_parent"_a, "viewport_width"_a, "viewport_height"_a, "clip"_a, py::return_value_policy::reference)
        .def(
            "subsurface_p",
            [](Surface& aSurface, double x, double y, double width, double sub_width, double sub_height, bool clip) -> Surface& {
                return aSurface.subsurface({x, y}, Pixels{width}, Size{sub_width, sub_height}, clip);
            },
            "origin_x_pixels"_a, "origin_y_pixels"_a, "width_in_parent"_a, "viewport_width"_a, "viewport_height"_a, "clip"_a, py::return_value_policy::reference)
        .def("new_page", &Surface::new_page)
        .def(
            "line_p",
            [](Surface& aSurface, double x1, double y1, double x2, double y2, std::string color, double width) {
                aSurface.line({x1, y1}, {x2, y2}, Color(color), Pixels{width});
            },
            "x1"_a, "y1"_a, "x2"_a, "y2"_a, "color"_a, "width"_a)
        .def(
            "line_s",
            [](Surface& aSurface, double x1, double y1, double x2, double y2, std::string color, double width) {
                aSurface.line({x1, y1}, {x2, y2}, Color(color), Scaled{width});
            },
            "x1"_a, "y1"_a, "x2"_a, "y2"_a, "color"_a, "width"_a)
        .def(
            "rectangle",
            [](Surface& aSurface, double x1, double y1, double width, double height, std::string color, double outline_width) {
                aSurface.rectangle({x1, y1}, {width, height}, Color(color), Pixels{outline_width});
            },
            "x"_a, "y"_a, "width"_a, "height"_a, "color"_a, "outline_width"_a)
        .def(
            "rectangle_filled",
            [](Surface& aSurface, double x1, double y1, double width, double height, std::string outline_color, double outline_width, std::string fill_color) {
                aSurface.rectangle_filled({x1, y1}, {width, height}, Color(outline_color), Pixels{outline_width}, Color(fill_color));
            },
            "x"_a, "y"_a, "width"_a, "height"_a, "outline_color"_a, "outline_width"_a, "fill_color"_a)
        .def(
            "circle_p",
            [](Surface& aSurface, double x, double y, double diameter, std::string outline_color, double width, double aspect, double rotation) {
                aSurface.circle({x, y}, Pixels{diameter}, Aspect{aspect}, Rotation{rotation}, Color(outline_color), Pixels{width});
            },
            "x"_a, "y"_a, "diameter"_a, "outline_color"_a, "width"_a, "aspect"_a = 1.0, "rotation"_a = 0.0)
        .def(
            "circle_s",
            [](Surface& aSurface, double x, double y, double diameter, std::string outline_color, double width, double aspect, double rotation) {
                aSurface.circle({x, y}, Scaled{diameter}, Aspect{aspect}, Rotation{rotation}, Color(outline_color), Pixels{width});
            },
            "x"_a, "y"_a, "diameter"_a, "outline_color"_a, "width"_a, "aspect"_a = 1.0, "rotation"_a = 0.0)
        .def(
            "circle_filled_p",
            [](Surface& aSurface, double x, double y, double diameter, std::string outline_color, double width, std::string fill_color, double aspect, double rotation) {
                aSurface.circle_filled({x, y}, Pixels{diameter}, Aspect{aspect}, Rotation{rotation}, Color(outline_color), Pixels{width}, acmacs::surface::Dash::NoDash, Color(fill_color));
            },
            "x"_a, "y"_a, "diameter"_a, "outline_color"_a, "width"_a, "fill_color"_a, "aspect"_a = 1.0, "rotation"_a = 0.0)
        .def(
            "circle_filled_s",
            [](Surface& aSurface, double x, double y, double diameter, std::string outline_color, double width, std::string fill_color, double aspect, double rotation) {
                aSurface.circle_filled({x, y}, Scaled{diameter}, Aspect{aspect}, Rotation{rotation}, Color(outline_color), Pixels{width}, acmacs::surface::Dash::NoDash, Color(fill_color));
            },
            "x"_a, "y"_a, "diameter"_a, "outline_color"_a, "width"_a, "fill_color"_a, "aspect"_a = 1.0, "rotation"_a = 0.0)
        .def(
            "square_filled_p",
            [](Surface& aSurface, double x, double y, double side, std::string outline_color, double width, std::string fill_color, double aspect, double rotation) {
                aSurface.square_filled({x, y}, Pixels{side}, Aspect{aspect}, Rotation{rotation}, Color(outline_color), Pixels{width}, Color(fill_color));
            },
            "x"_a, "y"_a, "side"_a, "outline_color"_a, "width"_a, "fill_color"_a, "aspect"_a = 1.0, "rotation"_a = 0.0)
        .def(
            "square_filled_s",
            [](Surface& aSurface, double x, double y, double side, std::string outline_color, double width, std::string fill_color, double aspect, double rotation) {
                aSurface.square_filled({x, y}, Scaled{side}, Aspect{aspect}, Rotation{rotation}, Color(outline_color), Pixels{width}, Color(fill_color));
            },
            "x"_a, "y"_a, "side"_a, "outline_color"_a, "width"_a, "fill_color"_a, "aspect"_a = 1.0, "rotation"_a = 0.0)
        .def(
            "path_outline",
            [](Surface& aSurface, const std::vector<double>& coordinates, std::string outline_color, double outline_width, bool close) {
                if (!coordinates.empty())
                    aSurface.path_outline(&*coordinates.begin(), &*coordinates.end(), Color(outline_color), Pixels{outline_width}, close);
                else
                    AD_WARNING("Surface.path_outline: empty path");
            },
            "path"_a, "outline_color"_a, "outline_width"_a, "close"_a = false)
        .def(
            "path_fill", [](Surface& aSurface, const std::vector<double>& coordinates, std::string fill_color) { aSurface.path_fill(&*coordinates.begin(), &*coordinates.end(), Color(fill_color)); },
            "path"_a, "fill_color"_a)
        .def(
            "path_outline_negative_move",
            [](Surface& aSurface, const std::vector<double>& coordinates, std::string outline_color, double outline_width, bool close) {
                aSurface.path_outline_negative_move(&*coordinates.begin(), &*coordinates.end(), Color(outline_color), Pixels{outline_width}, close);
            },
            "path"_a, "outline_color"_a, "outline_width"_a, "close"_a = false)
        .def(
            "path_fill_negative_move",
            [](Surface& aSurface, const std::vector<double>& coordinates, std::string fill_color) { aSurface.path_fill_negative_move(&*coordinates.begin(), &*coordinates.end(), Color(fill_color)); },
            "path"_a, "fill_color"_a)
        .def(
            "border",
            [](Surface& aSurface, std::string color, double width) {
                const auto& v = aSurface.viewport();
                aSurface.rectangle(v.origin, v.size, Color(color), Pixels{width * 2});
            },
            "color"_a, "width"_a)
        .def(
            "background",
            [](Surface& aSurface, std::string color) {
                const auto& v = aSurface.viewport();
                aSurface.rectangle_filled(v.origin, v.size, Color(color), Pixels{0}, Color(color));
            },
            "color"_a)
        .def(
            "text_p",
            [](Surface& aSurface, double x, double y, std::string text, std::string color, double size, double rotation) {
                aSurface.text({x, y}, text, Color(color), Pixels{size}, TextStyle(), Rotation(rotation));
            },
            "x"_a, "y"_a, "text"_a, "color"_a, "size"_a, "rotation"_a = 0)
        .def(
            "text_s",
            [](Surface& aSurface, double x, double y, std::string text, std::string color, double size, double rotation) {
                aSurface.text({x, y}, text, Color(color), Scaled{size}, TextStyle(), Rotation(rotation));
            },
            "x"_a, "y"_a, "text"_a, "color"_a, "size"_a, "rotation"_a = 0)
        .def(
            "text_right_aligned_p",
            [](Surface& aSurface, double x, double y, std::string text, std::string color, double size, double rotation) {
                aSurface.text_right_aligned({x, y}, text, Color(color), Pixels{size}, TextStyle(), Rotation(rotation));
            },
            "x"_a, "y"_a, "text"_a, "color"_a, "size"_a, "rotation"_a = 0)
        .def(
            "text_right_aligned_s",
            [](Surface& aSurface, double x, double y, std::string text, std::string color, double size, double rotation) {
                aSurface.text_right_aligned({x, y}, text, Color(color), Scaled{size}, TextStyle(), Rotation(rotation));
            },
            "x"_a, "y"_a, "text"_a, "color"_a, "size"_a, "rotation"_a = 0);

    py::class_<acmacs::surface::internal_1::Cairo, acmacs::surface::Surface>(mdl, "SurfaceCairo");

    py::class_<acmacs::surface::PdfCairo, acmacs::surface::internal_1::Cairo>(mdl, "PdfCairo")
        .def(py::init([](py::object path, double width, double height, double viewport_width) {
                 const std::string filename = py::str(path);
                 return new acmacs::surface::PdfCairo(filename, width, height, viewport_width);
             }),
             "filename"_a, "width"_a, "height"_a, "viewport_width"_a = 1000.0) //
        ;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
