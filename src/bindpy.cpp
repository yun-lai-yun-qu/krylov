
#include <pybind11/functional.h>
#include <pybind11/numpy.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <sstream>
#include <vector>

#include "boundary_conds.hpp"
#include "constants.hpp"
#include "mesh.hpp"
#include "solver.hpp"
#include "space_disc.hpp"
#include "time_disc.hpp"

namespace py = pybind11;

// This just exports all the objects needed in Python
PYBIND11_MODULE(krylov, module) {
  module.doc() = "C++ Solvers for the Incompressible Energy Equation";
  py::class_<Mesh> mesh(module, "Mesh");
  mesh.def(py::init<std::pair<real, real>, std::pair<real, real>, size_t,
                    size_t, std::function<triple(real, real)>>())
      .def(py::init<std::pair<real, real>, std::pair<real, real>, size_t,
                    size_t>());
  mesh.def("__getitem__",
           [](const Mesh &m, std::pair<int, int> p) {
             return m.Temp(p.first, p.second);
           })
      .def("__setitem__",
           [](Mesh &m, std::pair<int, int> p, real val) {
             m.Temp(p.first, p.second) = val;
             return m;
           })
      .def("cells_x", &Mesh::cells_x)
      .def("cells_y", &Mesh::cells_y)
      .def("median_x", &Mesh::median_x)
      .def("median_y", &Mesh::median_y)
      .def("dx", &Mesh::dx)
      .def("dy", &Mesh::dy)
      .def("Temp_array",
           [](Mesh &m) {
             return py::array((m.cells_x() + 2 * m.ghost_cells) *
                                  (m.cells_y() + 2 * m.ghost_cells),
                              m.data_Temp())
                 .attr("reshape")(m.cells_x() + 2 * m.ghost_cells,
                                  m.cells_y() + 2 * m.ghost_cells);
           })
      .def("grid_x",
           [](Mesh &m) {
             xt::xtensor<real, 2> grid(std::array<size_t, 2>{
                 static_cast<size_t>(m.cells_x() + 2 * m.ghost_cells),
                 static_cast<size_t>(m.cells_y() + 2 * m.ghost_cells)});
             for (int i = 0; i < m.cells_x() + 2 * m.ghost_cells; i++) {
               for (int j = 0; j < m.cells_y() + 2 * m.ghost_cells; j++) {
                 grid(i, j) = m.median_x(i - m.ghost_cells);
               }
             }
             return py::array((m.cells_x() + 2 * m.ghost_cells) *
                                  (m.cells_y() + 2 * m.ghost_cells),
                              grid.data())
                 .attr("reshape")(m.cells_x() + 2 * m.ghost_cells,
                                  m.cells_y() + 2 * m.ghost_cells);
           })
      .def("grid_y", [](Mesh &m) {
        xt::xtensor<real, 2> grid(std::array<size_t, 2>{
            static_cast<size_t>(m.cells_x() + 2 * m.ghost_cells),
            static_cast<size_t>(m.cells_y() + 2 * m.ghost_cells)});
        for (int i = 0; i < m.cells_x() + 2 * m.ghost_cells; i++) {
          for (int j = 0; j < m.cells_y() + 2 * m.ghost_cells; j++) {
            grid(i, j) = m.median_y(j - m.ghost_cells);
          }
        }
        return py::array((m.cells_x() + 2 * m.ghost_cells) *
                             (m.cells_y() + 2 * m.ghost_cells),
                         grid.data())
            .attr("reshape")(m.cells_x() + 2 * m.ghost_cells,
                             m.cells_y() + 2 * m.ghost_cells);
      });

  enum class BCSideVert { Left, Right };

  py::class_<BoundaryCondition<Mesh::vert_view>,
             std::shared_ptr<BoundaryCondition<Mesh::vert_view>>>
      bc_vert(module, "BoundaryConditionVert");
  bc_vert
      .def("Temp_bc",
           [](Mesh &m, const BCSideVert side, const BCType type,
              const std::function<real(real, real, real)> val) {
             assert(side == BCSideVert::Left || side == BCSideVert::Right);
             if (side == BCSideVert::Left) {
               return BoundaryCondition(
                   m.ghostcells_left_Temp(), m.bndrycells_left_Temp(),
                   [m](int j) { return m.left_x(0); },
                   [m](int j) { return m.median_y(j); }, val, m.dy(), type);
             } else {
               return BoundaryCondition(
                   m.ghostcells_right_Temp(), m.bndrycells_right_Temp(),
                   [m](int j) { return m.left_x(m.cells_x()); },
                   [m](int j) { return m.median_y(j); }, val, m.dy(), type);
             }
           })
      .def("vel_u_bc",
           [](Mesh &m, const BCSideVert side, const BCType type,
              const std::function<real(real, real, real)> val) {
             assert(side == BCSideVert::Left || side == BCSideVert::Right);
             if (side == BCSideVert::Left) {
               return BoundaryCondition(
                   m.ghostcells_left_vel_u(), m.bndrycells_left_vel_u(),
                   [m](int j) { return m.left_x(0); },
                   [m](int j) { return m.median_y(j); }, val, m.dy(), type);
             } else {
               return BoundaryCondition(
                   m.ghostcells_right_vel_u(), m.bndrycells_right_vel_u(),
                   [m](int j) { return m.left_x(m.cells_x()); },
                   [m](int j) { return m.median_y(j); }, val, m.dy(), type);
             }
           })
      .def("vel_v_bc",
           [](Mesh &m, const BCSideVert side, const BCType type,
              const std::function<real(real, real, real)> val) {
             assert(side == BCSideVert::Left || side == BCSideVert::Right);
             if (side == BCSideVert::Left) {
               return BoundaryCondition(
                   m.ghostcells_left_vel_v(), m.bndrycells_left_vel_v(),
                   [m](int j) { return m.left_x(0); },
                   [m](int j) { return m.median_y(j); }, val, m.dy(), type);
             } else {
               return BoundaryCondition(
                   m.ghostcells_right_vel_v(), m.bndrycells_right_vel_v(),
                   [m](int j) { return m.left_x(m.cells_x()); },
                   [m](int j) { return m.median_y(j); }, val, m.dy(), type);
             }
           })
      .def("apply", &BoundaryCondition<Mesh::vert_view>::apply);

  py::enum_<BCSideVert>(bc_vert, "BCSide")
      .value("left", BCSideVert::Left)
      .value("right", BCSideVert::Right)
      .export_values();

  enum class BCSideHoriz { Top, Bottom };

  py::class_<BoundaryCondition<Mesh::horiz_view>,
             std::shared_ptr<BoundaryCondition<Mesh::horiz_view>>>
      bc_horiz(module, "BoundaryConditionHoriz");
  bc_horiz
      .def("Temp_bc",
           [](Mesh &m, const BCSideHoriz side, const BCType type,
              const std::function<real(real, real, real)> val) {
             assert(side == BCSideHoriz::Top || side == BCSideHoriz::Bottom);
             if (side == BCSideHoriz::Top) {
               return BoundaryCondition(
                   m.ghostcells_top_Temp(), m.bndrycells_top_Temp(),
                   [m](int i) { return m.median_x(i); },
                   [m](int i) { return m.bottom_y(m.cells_y()); }, val, m.dx(),
                   type);
             } else {
               return BoundaryCondition(
                   m.ghostcells_bottom_Temp(), m.bndrycells_bottom_Temp(),
                   [m](int i) { return m.median_x(i); },
                   [m](int i) { return m.bottom_y(0); }, val, m.dx(), type);
             }
           })
      .def("vel_u_bc",
           [](Mesh &m, const BCSideHoriz side, const BCType type,
              const std::function<real(real, real, real)> val) {
             assert(side == BCSideHoriz::Top || side == BCSideHoriz::Bottom);
             if (side == BCSideHoriz::Top) {
               return BoundaryCondition(
                   m.ghostcells_top_vel_u(), m.bndrycells_top_vel_u(),
                   [m](int i) { return m.median_x(i); },
                   [m](int i) { return m.bottom_y(m.cells_y()); }, val, m.dx(),
                   type);
             } else {
               return BoundaryCondition(
                   m.ghostcells_bottom_vel_u(), m.bndrycells_bottom_vel_u(),
                   [m](int i) { return m.median_x(i); },
                   [m](int i) { return m.bottom_y(0); }, val, m.dx(), type);
             }
           })
      .def("vel_v_bc",
           [](Mesh &m, const BCSideHoriz side, const BCType type,
              const std::function<real(real, real, real)> val) {
             assert(side == BCSideHoriz::Top || side == BCSideHoriz::Bottom);
             if (side == BCSideHoriz::Top) {
               return BoundaryCondition(
                   m.ghostcells_top_vel_v(), m.bndrycells_top_vel_v(),
                   [m](int i) { return m.median_x(i); },
                   [m](int i) { return m.bottom_y(m.cells_y()); }, val, m.dx(),
                   type);
             } else {
               return BoundaryCondition(
                   m.ghostcells_bottom_vel_v(), m.bndrycells_bottom_vel_v(),
                   [m](int i) { return m.median_x(i); },
                   [m](int i) { return m.bottom_y(0); }, val, m.dx(), type);
             }
           })
      .def("apply", &BoundaryCondition<Mesh::horiz_view>::apply);

  py::enum_<BCSideHoriz>(bc_horiz, "BCSide")
      .value("top", BCSideHoriz::Top)
      .value("bottom", BCSideHoriz::Bottom)
      .export_values();

  py::enum_<BCType>(module, "BCType")
      .value("dirichlet", BCType::Dirichlet)
      .value("neumann", BCType::Neumann)
      .export_values();

  py::class_<SecondOrderCentered> space_disc(module, "SecondOrderCentered");
  space_disc.def(py::init<const real, const real, const real, const real>());

  py::class_<TimeDisc<SecondOrderCentered>,
             std::shared_ptr<TimeDisc<SecondOrderCentered>>>(module,
                                                             "_TimeDiscBase")
      .def("mesh", &TimeDisc<SecondOrderCentered>::mesh)
      .def("cur_time", &TimeDisc<SecondOrderCentered>::cur_time);
  py::class_<ImplicitEuler<SecondOrderCentered, LUSolver>,
             std::shared_ptr<ImplicitEuler<SecondOrderCentered, LUSolver>>,
             TimeDisc<SecondOrderCentered>>
      ie_solver(module, "ImplicitEulerLUSolver");
  ie_solver
      .def(py::init<const std::pair<real, real>, const std::pair<real, real>,
                    const size_t, const size_t,
                    std::function<triple(real, real)>,
                    const SecondOrderCentered &,
                    std::pair<BCType, std::function<real(real, real, real)>>,
                    std::pair<BCType, std::function<real(real, real, real)>>,
                    std::pair<BCType, std::function<real(real, real, real)>>,
                    std::pair<BCType, std::function<real(real, real, real)>>,
                    std::pair<BCType, std::function<real(real, real, real)>>,
                    std::pair<BCType, std::function<real(real, real, real)>>,
                    std::pair<BCType, std::function<real(real, real, real)>>,
                    std::pair<BCType, std::function<real(real, real, real)>>,
                    std::pair<BCType, std::function<real(real, real, real)>>,
                    std::pair<BCType, std::function<real(real, real, real)>>,
                    std::pair<BCType, std::function<real(real, real, real)>>,
                    std::pair<BCType, std::function<real(real, real, real)>>>())
      .def("problem_configuration",
           []() {
             const std::pair<real, real> c1{0.0, 0.0}, c2{1.0, 1.0};
             const size_t cells_x = 20, cells_y = 10;
             auto initial = [](real x, real y) {
               const real Temp = std::cos(pi * x) * std::sin(pi * y);
               const real u_vel = y * std::sin(pi * x);
               const real v_vel = x * std::cos(pi * y);
               return std::tuple{Temp, u_vel, v_vel};
             };
             constexpr real reynolds = 25.0;
             constexpr real prandtl = 0.7;
             constexpr real eckert = 0.1;
             constexpr real diffusion = 1.0;
             const SecondOrderCentered space(diffusion, reynolds, prandtl,
                                             eckert);
             const std::pair<BCType, std::function<real(real, real, real)>>
                 hg_default{BCType::Dirichlet,
                            [](real, real, real) { return 0.0; }};

             const std::pair<BCType, std::function<real(real, real, real)>>
                 bottom(hg_default);
             const std::pair<BCType, std::function<real(real, real, real)>> top{
                 BCType::Dirichlet, [](real, real, real) { return 1.0; }};
             const std::pair<BCType, std::function<real(real, real, real)>>
                 left{BCType::Dirichlet, [](real, real y, real) { return y; }};
             const std::pair<BCType, std::function<real(real, real, real)>>
                 right(hg_default);

             return ImplicitEuler<SecondOrderCentered, LUSolver>(
                 c1, c2, cells_x, cells_y, initial, space, left, right, top,
                 bottom, hg_default, hg_default, hg_default, hg_default,
                 hg_default, hg_default, hg_default, hg_default);
           })
      .def("timestep", &ImplicitEuler<SecondOrderCentered, LUSolver>::timestep)
      .def("system", &ImplicitEuler<SecondOrderCentered, LUSolver>::sys)
      .def("solution", &ImplicitEuler<SecondOrderCentered, LUSolver>::sol)
      .def("delta", &ImplicitEuler<SecondOrderCentered, LUSolver>::delta);

  py::class_<matrix> matrix_obj(module, "Matrix");
  matrix_obj.def(py::init<matrix::shape_type>())
      .def("shape", [](const matrix &m) { return m.shape(); })
      .def("__getitem__",
           [](const matrix &m, std::pair<int, int> idx) {
             assert(idx.first >= 0);
             assert(idx.first < m.shape()[0]);
             assert(idx.second >= 0);
             assert(idx.second < m.shape()[1]);
             return m(idx.first, idx.second);
           })
      .def("__setitem__",
           [](matrix &dest, std::pair<int, int> idx, real val) {
             assert(idx.first >= 0);
             assert(idx.first < dest.shape()[0]);
             assert(idx.second >= 0);
             assert(idx.second < dest.shape()[1]);
             dest(idx.first, idx.second) = val;
             return val;
           })
      .def("to_array",
           [](const matrix &src) {
             return py::array(src.shape()[0] * src.shape()[1], src.data())
                 .attr("reshape")(src.shape()[0], src.shape()[1]);
           })
      .def("from_array", [](const py::array_t<real> &np_arr) {
        matrix m(
            matrix::shape_type{static_cast<unsigned long>(np_arr.shape()[0]),
                               static_cast<unsigned long>(np_arr.shape()[1])});
        auto src = np_arr.unchecked<2>();
        for (int i = 0; i < src.shape(0); i++) {
          for (int j = 0; j < src.shape(1); j++) {
            m(i, j) = src(i, j);
          }
        }
        return m;
      });
  py::class_<vector> vector_obj(module, "Vector");
  vector_obj.def(py::init<vector::shape_type>())
      .def("shape", [](const vector &v) { return v.shape(); })
      .def("__getitem__",
           [](const vector &v, int idx) {
             assert(idx >= 0);
             assert(idx < v.shape()[0]);
             return v(idx);
           })
      .def("__setitem__",
           [](vector &dest, int idx, real val) {
             assert(idx >= 0);
             assert(idx < dest.shape()[0]);
             dest(idx) = val;
             return val;
           })
      .def("to_array",
           [](const vector &src) {
             return py::array(src.shape()[0], src.data());
           })
      .def("from_array", [](const py::array_t<real> &np_arr) {
        vector v(
            vector::shape_type{static_cast<unsigned long>(np_arr.shape()[0])});
        auto src = np_arr.unchecked<1>();
        for (int i = 0; i < src.shape(0); i++) {
          v(i) = src(i);
        }
        return v;
      });

  py::class_<LUSolver> lu_solver(module, "LUSolver");
  lu_solver.def(py::init<unsigned long>())
      .def("solve", &LUSolver::solve)
      .def("lu_decomp", &LUSolver::lu_decomp);
}
