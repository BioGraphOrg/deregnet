#include <vector>
#include <map>

#include <gurobi_c++.h>

#include <grbfrc/CharnesCooper.h>

namespace grbfrc
{

CharnesCooper::CharnesCooper(FMILP& fmipRef)
             : fmip { &fmipRef },
               transformed { false },
               transformation { GRBModel((fmip->baseModel)->getEnv()) }
 {
  if (!fmip->isFLP()) { printInvalidity(); invalid = true; }
  else { invalid = false; fmip->update(); }
 }

void CharnesCooper::printInvalidity()
 {
   std::cout << "\n!!! === Not a FLP === !!!\n\n"; 
 }

void CharnesCooper::transform()
 {
  // create t variable corr. to t = 1/(ex + f) // set objective to max {cy + dt} //
  t = transformation.addVar(0.0, GRB_INFINITY, 0.0, GRB_CONTINUOUS, "t");
  // create y variables corr. to y = tx //
  for (auto var : fmip->vars) {
        y.push_back( new GRBVar( transformation.addVar(-GRB_INFINITY, GRB_INFINITY, 0.0, GRB_CONTINUOUS) ) );
        transformation.update();
        double lb { var->get(GRB_DoubleAttr_LB) };
        double ub { var->get(GRB_DoubleAttr_UB) };
        transformation.addConstr(*(y.back()) - lb*t >= 0.0);
        transformation.addConstr(*(y.back()) - ub*t <= 0.0);
  }
  transformation.update();
  // objective of the FLP: max (cx + d)/(ex + f) //
  FMILPObj& objective = fmip->objective;
  GRBLinExpr& objNumerator = objective.numerator;
  double d { objNumerator.getConstant() };
  GRBLinExpr& objDenominator = objective.denominator;
  double f { objDenominator.getConstant() };
  int objSense = objective.sense;
  // set objective of transform to cy + dt
  GRBLinExpr transformObj { d * t };
  for (unsigned int i = 0; i < objNumerator.size(); i++)
   {
    GRBVar var = objNumerator.getVar(i);
    int j = getIndex(var);
    double coeff = objNumerator.getCoeff(i);
    transformObj += coeff * ( *y[j] );
   }
  transformation.setObjective(transformObj, objSense);
  // add ey + ft == 1 constraint //
  GRBLinExpr lhs { f * t };
  for (unsigned int i = 0; i < objDenominator.size(); i++)
   {
    GRBVar var = objDenominator.getVar(i);
    int j = getIndex(var);
    double coeff = objDenominator.getCoeff(i);
    lhs += coeff * ( *y[j] );
   }
  transformation.addConstr(lhs, GRB_EQUAL, 1.0);
  // constraints of the FLP: Ax {<=,=,>=} b //
  GRBConstr* constrs { fmip->getConstrs() };
  int numConstrs { fmip->get(GRB_IntAttr_NumConstrs) };
  // add Ay - bt {<=,=,>=} 0 constraints //
  for (int i = 0; i < numConstrs; i++)
   {
    GRBLinExpr origLhs { fmip->getRow(*constrs) };
    double b { constrs->get(GRB_DoubleAttr_RHS) };
    lhs = - b * t ;
    for (unsigned int i = 0; i < origLhs.size(); i++)
     {
      GRBVar var = origLhs.getVar(i);
      int j = getIndex(var);
      double coeff = origLhs.getCoeff(i);
      lhs += coeff * ( *y[j] );
     }
    char constrSense { constrs->get(GRB_CharAttr_Sense) };
    transformation.addConstr(lhs, constrSense, 0.0);
    constrs++;
   }
  transformation.update();
 }

GRBModel CharnesCooper::getTransform()
 {
  if (invalid) 
   {
    printInvalidity();
    return transformation;
   }
  else if (transformed) return transformation; 
  else 
   {
    transform();
    return transformation;
   }
 }

void CharnesCooper::solveTransform()
 {
  if (invalid) printInvalidity(); 
  else
   {
    try
     {
      transformation.optimize();
      std::cout << std::endl;
     }
    catch (GRBException e)
     { 
      std::cout << "Gurobi error: " << e.getMessage() << "\n\n";
     }
    catch (...)
     {
      std::cout << "Error while attempting to optimize transform ... \n\n";
     }
   }
 }

void CharnesCooper::solveTransform(GRBCallback& callback)
 {
  if (invalid) printInvalidity();
  else
   {
    try
     {
      transformation.setCallback(&callback);
      transformation.optimize();
      std::cout << std::endl;
     }
    catch (GRBException e)
     {
      std::cout << "Gurobi error: " << e.getMessage() << "\n\n";
     }
    catch (...)
     {
      std::cout << "Error while attempting to optimize transform ... \n\n";
     }
   }
 }

void CharnesCooper::run(int time_limit)
 {
  if (invalid) printInvalidity();
  else
   {
    std::cout << "\n=========== solving FLP via Charnes-Cooper transform ===========\n\n";
    if (!transformed) transform();
    solveTransform();
    backTransformSolution();
   }
 }

void CharnesCooper::run(GRBCallback& callback, int time_limit)
 {
  if (invalid) printInvalidity();
  else
   {
    std::cout << "\n=========== solving FLP via Charnes-Cooper transform ===========\n\n";
    if (!transformed) transform();
    solveTransform(callback);
    backTransformSolution();
   }
 }

void CharnesCooper::writeSolution()
 {
  if (fmip->solution) *(fmip->solution) = solution;
  else if (!fmip->solution) fmip->solution = new FMILPSol(solution);
  else std::cout << "No solution avaiable!" << std::endl;
 }


FMILPSol CharnesCooper::getSolution()
 {
  return solution;
 }

void CharnesCooper::backTransformSolution()
 {
  int status { transformation.get(GRB_IntAttr_Status) };
  if (status == GRB_OPTIMAL)
   {
    solution.objVal = transformation.get(GRB_DoubleAttr_ObjVal);
    double tvalue { t.get(GRB_DoubleAttr_X) };
    for (int i = 0; i < fmip->get(GRB_IntAttr_NumVars); i++)
     {
      double value { y[i]->get(GRB_DoubleAttr_X) };
      solution.varVals.push_back( value / tvalue );
     }
   }
  else std::cout << "GRB_Status : " << transformation.get(GRB_IntAttr_Status) << std::endl;
 }

int CharnesCooper::getIndex(GRBVar& var)
 {
  for (int i = 0; i < fmip->get(GRB_IntAttr_NumVars); i++)
    if (var.sameAs(*(fmip->vars[i]))) return i;
  return -1;
 }

}   //  namespace grbfrc