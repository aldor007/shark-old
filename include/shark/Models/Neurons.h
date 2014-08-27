/*!
 * 
 *
 * \brief       -
 *
 * \author      O.Krause
 * \date        2011
 *
 *
 * \par Copyright 1995-2014 Shark Development Team
 * 
 * <BR><HR>
 * This file is part of Shark.
 * <http://image.diku.dk/shark/>
 * 
 * Shark is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published 
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Shark is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with Shark.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef MODELS_NEURONS_H
#define MODELS_NEURONS_H
 
#include <shark/LinAlg/Base.h>

 
namespace shark{
namespace detail{
	///\brief Baseclass for all Neurons. it defines y=operator(x) for evaluation and derivative(y) for the derivative of the sigmoid.
	///
	///You need to provide a public static member function function() and functionDerivative() in the derived class.
	///Those functions calculate value and derivative for a single input.
	///Due to template magic, the neurons can either use vectors or matrices as input.
	///Additionally, they avoid temporary values completely using ublas magic.
	///Usage: 
	///struct Neuron:public NeuronBase<Neuron> { 
	///    static double function(double x){return ...}
        ///    static double functionDerivative(double y){return ...}
	///};
	template<class Derived>
	//again, one step ahead using templates!
	class NeuronBase{
	private:
		template<class T>
		struct Function{
			typedef T argument_type;
			typedef argument_type result_type;
			static const bool zero_identity = false;

			result_type operator()(argument_type x)const{
				return Derived::function(x);
			}
		};
		template<class T>
		struct FunctionDerivative{
			typedef T argument_type;
			typedef argument_type result_type;
			static const bool zero_identity = false;

			result_type operator()(argument_type x)const{
				return Derived::functionDerivative(x);
			}
		};
	public:
		
		///for a given input vector, calculates the elementwise application of the sigmoid function defined by Derived.
		template<class E>
		blas::vector_unary<E, Function<typename E::value_type> > operator()(blas::vector_expression<E> const& x)const{
			typedef Function<typename E::value_type> functor_type;
			return blas::vector_unary<E, functor_type >(x,functor_type());
		}
		///for a given batch of input vectors, calculates the elementwise application of the sigmoid function defined by Derived.
		template<class E>
		blas::matrix_unary<E, Function<typename E::value_type> > operator()(blas::matrix_expression<E> const& x)const{
			typedef Function<typename E::value_type> functor_type;
			return blas::matrix_unary<E, functor_type >(x,functor_type());
		}
		///Calculates the elementwise application of the sigmoid function derivative defined by Derived.
		///It's input is a matrix or vector of previously calculated neuron responses generated by operator()
		template<class E>
		blas::vector_unary<E, FunctionDerivative<typename E::value_type> > derivative(blas::vector_expression<E> const& x)const{
			typedef FunctionDerivative<typename E::value_type> functor_type;
			return blas::vector_unary<E, functor_type >(x,functor_type());
		}
		///Calculates the elementwise application of the sigmoid function derivative defined by Derived.
		///It's input is a matrix or vector of previously calculated neuron responses generated by operator()
		template<class E>
		blas::matrix_unary<E, FunctionDerivative<typename E::value_type> > derivative(blas::matrix_expression<E> const& x)const{
			typedef FunctionDerivative<typename E::value_type> functor_type;
			return blas::matrix_unary<E, functor_type >(x,functor_type());
		}
	};
}
	
///\brief Neuron which computes the Logistic (logistic) function with range [0,1].
///
///The Logistic function is 
///\f[ f(x)=\frac 1 {1+exp^(-x)}\f]
///it's derivative can be computed as
///\f[ f'(x)= 1-f(x)^2 \f]
struct LogisticNeuron : public detail::NeuronBase<LogisticNeuron>{
	template<class T>
	static T function(T x){
		return sigmoid(x);
	}
	template<class T>
	static T functionDerivative(T y){
		return y * (1 - y);
	}
};
///\brief Neuron which computes the hyperbolic tangenst with range [-1,1].
///
///The Tanh function is 
///\f[ f(x)=\tanh(x) = \frac 2 {1+exp^(-2x)}-1 \f]
///it's derivative can be computed as
///\f[ f'(x)= f(x)(1-f(x)) \f]
struct TanhNeuron: public detail::NeuronBase<TanhNeuron>{
	template<class T>
	static T function(T x){
		return std::tanh(x);
	}
	template<class T>
	static T functionDerivative(T y){
		return 1.0 - y*y;
	}
};
///\brief Linear activation Neuron. 
struct LinearNeuron: public detail::NeuronBase<LinearNeuron>{
	template<class T>
	static T function(T x){
		return x;
	}
	template<class T>
	static T functionDerivative(T y){
		return 1.0;
	}
};

///\brief Rectifier Neuron f(x) = max(0,x)
struct RectifierNeuron: public detail::NeuronBase<RectifierNeuron>{
	template<class T>
	static T function(T x){
		return std::max<T>(0,x);
	}
	template<class T>
	static T functionDerivative(T y){
		if(y == 0) 
			return T(0);
		return T(1);
	}
};

///\brief Fast sigmoidal function, which does not need to compute an exponential function.
///
///It is defined as
///\f[ f(x)=\frac x {1+|x|}\f]
///it's derivative can be computed as
///\f[ f'(x)= (1 - |f(x)|)^2 \f]
struct FastSigmoidNeuron: public detail::NeuronBase<FastSigmoidNeuron>{
	template<class T>
	static T function(T x){
		return x/(1+std::abs(x));
	}
	template<class T>
	static T functionDerivative(T y){
		return sqr(1.0 - std::abs(y));
	}
};


/// \brief Wraps a given neuron type and implements dropout for it
///
/// The function works by setting the output randomly to 0 with a 50% chance.
/// The function assumes for the wrapped neuron type that the derivative
/// for all points for which the output is 0, is 0. This is true for the LogisticNeuron,
/// FastSigmoidNeuron and RectifierNeuron.
template<class Neuron>
struct DropoutNeuron: public detail::NeuronBase<DropoutNeuron<Neuron> >{
	template<class T>
	static T function(T x){
		if(Rng::coinToss()){
			return T(0);
		}
		else{
			return Neuron::function(x);
		}
	}
	template<class T>
	static T functionDerivative(T y){
		return Neuron::functionDerivative(y);
	}
};

}

#endif
