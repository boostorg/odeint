// Copyright 2026 Treston Cuzzort.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or https://www.boost.org/LICENSE_1_0.txt.

#include <boost/core/lightweight_test.hpp>
#include <boost/numeric/odeint/stepper/controlled_runge_kutta.hpp>
#include <boost/numeric/odeint/stepper/runge_kutta_dopri5.hpp>
#include <array>
#include <cstddef>
#include <vector>

namespace odeint = boost::numeric::odeint;

double derivative(double value, std::size_t index, double time)
{
    return (index + 1) * value * value + time;
}

struct counted_system
{
    unsigned* calls;

    template<class State, class Deriv>
    void operator()(const State& state, Deriv& dxdt, double time) const
    {
        ++*calls;
        for(std::size_t i = 0; i < state.size(); ++i)
            dxdt[i] = derivative(state[i], i, time);
    }
};

struct checked_error
{
    const double* time;
    unsigned* calls;

    template<class Algebra, class State, class Deriv, class Error>
    double error(Algebra&, const State& state, const Deriv& dxdt, Error&, double) const
    {
        // The checker must see the derivative at the beginning of the attempt,
        // even after do_step has calculated the derivative at the candidate end.
        for(std::size_t i = 0; i < state.size(); ++i)
            BOOST_TEST_EQ(dxdt[i], derivative(state[i], i, *time));
        return (++*calls % 4 == 1) ? 2.0 : 0.75;
    }
};

template<class State>
void resize_state(State&, std::size_t) {}

void resize_state(std::vector<double>& state, std::size_t size)
{
    state.resize(size);
}

template<class State>
State make_state()
{
    State state;
    resize_state(state, 2);
    for(std::size_t i = 0; i < state.size(); ++i)
        state[i] = 0.1 * (i + 1);
    return state;
}

template<class State>
struct types
{
    typedef odeint::runge_kutta_dopri5<State, double, State, double,
        typename odeint::algebra_dispatcher<State>::algebra_type,
        odeint::default_operations, odeint::always_resizer> error_stepper;
    typedef odeint::controlled_runge_kutta<error_stepper, checked_error,
        odeint::default_step_adjuster<double, double>, odeint::always_resizer> controlled;
};

template<class State>
void compare_attempts(double direction, bool resize)
{
    State input = make_state<State>();
    double time = 0.0, reference_time = 0.0;
    double dt = direction * 0.02, reference_dt = dt;
    unsigned calls = 0, reference_calls = 0, errors = 0, reference_errors = 0;
    checked_error checker = {&time, &errors};
    checked_error reference_checker = {&reference_time, &reference_errors};
    typename types<State>::controlled controlled(checker);
    typename types<State>::controlled reference(reference_checker);
    unsigned initializations = 1;

    for(unsigned attempt = 0; attempt < 24; ++attempt)
    {
        if(attempt == 5)
        {
            controlled.reset();
            ++initializations;
        }
        if(attempt == 9)
        {
            State dxdt = input;
            for(std::size_t i = 0; i < input.size(); ++i)
                dxdt[i] = derivative(input[i], i, time);
            controlled.adjust_size(input);
            controlled.initialize(dxdt);
        }
        if(attempt == 12)
        {
            typename types<State>::controlled copied(controlled);
            controlled = copied;
        }
        if(resize && attempt == 16)
        {
            resize_state(input, 3);
            input[2] = 0.3;
            ++initializations;
        }

        const State before = input;
        State output = input, reference_output = input;
        State dxdt = input, dxdt_out = input;
        for(std::size_t i = 0; i < input.size(); ++i)
            dxdt[i] = derivative(input[i], i, reference_time);

        const odeint::controlled_step_result result =
            controlled.try_step(counted_system{&calls}, input, time, output, dt);
        const odeint::controlled_step_result expected =
            reference.try_step(counted_system{&reference_calls}, input, dxdt,
                reference_time, reference_output, dxdt_out, reference_dt);

        BOOST_TEST_EQ(result, expected);
        BOOST_TEST_EQ(time, reference_time);
        BOOST_TEST_EQ(dt, reference_dt);
        BOOST_TEST_EQ(errors, reference_errors);
        BOOST_TEST_EQ(calls, 6 * (attempt + 1) + initializations);
        BOOST_TEST_EQ(reference_calls, 6 * (attempt + 1));
        for(std::size_t i = 0; i < input.size(); ++i)
        {
            BOOST_TEST_EQ(input[i], before[i]);
            // The proposed output is available even when error control rejects it.
            BOOST_TEST_EQ(output[i], reference_output[i]);
        }
        if(result == odeint::success)
            input = output;
    }
}

template<class State>
void check_step_limit(double direction)
{
    State input = make_state<State>(), output = input;
    for(std::size_t i = 0; i < output.size(); ++i) output[i] = -123.0;
    double time = 0.0;
    double dt = direction * 0.2;
    unsigned calls = 0, errors = 0;
    checked_error checker = {&time, &errors};
    typename types<State>::controlled controlled(
        checker, odeint::default_step_adjuster<double, double>(direction * 0.01));

    BOOST_TEST_EQ(controlled.try_step(counted_system{&calls}, input, time, output, dt), odeint::fail);
    BOOST_TEST_EQ(time, 0.0);
    BOOST_TEST_EQ(dt, direction * 0.01);
    BOOST_TEST_EQ(calls, 1u);
    BOOST_TEST_EQ(errors, 0u);
    for(std::size_t i = 0; i < output.size(); ++i) BOOST_TEST_EQ(output[i], -123.0);

    // The first actual attempt is rejected, then retried from the same state.
    BOOST_TEST_EQ(controlled.try_step(counted_system{&calls}, input, time, output, dt), odeint::fail);
    BOOST_TEST_EQ(time, 0.0);
    BOOST_TEST_EQ(calls, 7u);
    BOOST_TEST_EQ(errors, 1u);
    BOOST_TEST_EQ(controlled.try_step(counted_system{&calls}, input, time, output, dt), odeint::success);
    BOOST_TEST(time * direction > 0.0);
    BOOST_TEST_EQ(calls, 13u);
    BOOST_TEST_EQ(errors, 2u);
}

int main()
{
    compare_attempts<std::array<double, 2> >(1.0, false);
    compare_attempts<std::array<double, 2> >(-1.0, false);
    compare_attempts<std::vector<double> >(1.0, true);
    compare_attempts<std::vector<double> >(-1.0, true);
    check_step_limit<std::array<double, 2> >(1.0);
    check_step_limit<std::array<double, 2> >(-1.0);
    check_step_limit<std::vector<double> >(1.0);
    check_step_limit<std::vector<double> >(-1.0);
    return boost::report_errors();
}
