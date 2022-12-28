#include <cmath>
#include <iomanip>
#include <iostream>
#include <tuple>

float inline log_add(float x, float y) {
    if (y <= x) {
        return x + std::log1p(std::exp(y - x));
    } else {
        return y + std::log1p(std::exp(x - y));
    }
}

float* forward(
    const int* observations,
    const std::size_t num_observations,
    const float* transition,
    const float* emission,
    const float* initial,
    const std::size_t num_states,
    const std::size_t num_emissions) {
    // init the alpha matrix
    float* alpha = new float[num_states * num_observations]();
    for (std::size_t i = 0; i < num_states; ++i) {
        alpha[i * num_observations] = initial[i] + emission[i * num_emissions + observations[0]];
    }

    // compute the alpha matrix
    for (std::size_t t = 1; t < num_observations; ++t) {
        // now is observation t, and t is from 1 because we skip init observation
        for (std::size_t curr_state = 0; curr_state < num_states; ++curr_state) {
            float sum = -INFINITY;
            for (std::size_t prev_state = 0; prev_state < num_states; ++prev_state) {
                sum = log_add(sum, alpha[prev_state * num_observations + t - 1] + transition[prev_state * num_states + curr_state]);
            }
            alpha[curr_state * num_observations + t] = sum + emission[curr_state * num_emissions + observations[t]];
        }
    }

    return alpha;
}

float* backward(
    const int* observations,
    const std::size_t num_observations,
    const float* transition,
    const float* emission,
    const std::size_t num_states,
    const std::size_t num_emissions) {
    // init the beta matrix
    float* beta = new float[num_states * num_observations]();
    for (std::size_t i = 0; i < num_states; ++i) {
        beta[i * num_observations + num_observations - 1] = 0; // log(1)
    }

    // compute the beta matrix
    for (std::size_t t = num_observations - 2;; --t) {
        // now is observation t, and t is from (num_observations - 2) because we already init last observation
        for (std::size_t prev_state = 0; prev_state < num_states; ++prev_state) {
            float sum = -INFINITY;
            for (std::size_t curr_state = 0; curr_state < num_states; ++curr_state) {
                sum = log_add(sum, beta[curr_state * num_observations + t + 1] + transition[prev_state * num_states + curr_state] + emission[curr_state * num_emissions + observations[t + 1]]);
            }
            beta[prev_state * num_observations + t] = sum;
        }

        if (t == 0) {
            break;
        }
    }

    return beta;
}

float* compute_gamma(
    const float* alpha,
    const float* beta,
    const std::size_t num_observations,
    const std::size_t num_states) {
    float* gamma = new float[num_states * num_observations]();

    for (std::size_t t = 0; t < num_observations; ++t) {
        float sum = -INFINITY;
        for (std::size_t i = 0; i < num_states; ++i) {
            gamma[i * num_observations + t] = alpha[i * num_observations + t] + beta[i * num_observations + t];
            sum = log_add(sum, gamma[i * num_observations + t]);
        }

        for (std::size_t i = 0; i < num_states; ++i) {
            gamma[i * num_observations + t] -= sum;
        }
    }

    return gamma;
}

float* compute_xi(
    const float* alpha,
    const float* beta,
    const int* observations,
    const std::size_t num_observations,
    const float* transition,
    const float* emission,
    const std::size_t num_states,
    const std::size_t num_emissions) {
    float* xi = new float[num_states * num_states * num_observations]();

    for (std::size_t t = 0; t < num_observations - 1; ++t) {
        float sum = -INFINITY;
        for (std::size_t i = 0; i < num_states; ++i) {
            for (std::size_t j = 0; j < num_states; ++j) {
                xi[i * num_states * num_observations + j * num_observations + t] = alpha[i * num_observations + t] + transition[i * num_states + j] + emission[j * num_emissions + observations[t + 1]] + beta[j * num_observations + t + 1];
                sum = log_add(sum, xi[i * num_states * num_observations + j * num_observations + t]);
            }
        }

        for (std::size_t i = 0; i < num_states; ++i) {
            for (std::size_t j = 0; j < num_states; ++j) {
                xi[i * num_states * num_observations + j * num_observations + t] -= sum;
            }
        }
    }

    return xi;
}

void baum_welch(
    const int* observations,
    const std::size_t num_observations,
    float* initial,
    float* transition,
    float* emission,
    const std::size_t num_states,
    const std::size_t num_emissions,
    const float tolerance = 1e-7,
    const std::size_t max_iters = 1000) {
    // transform initial to log space
    float* log_initial = new float[num_states];
    for (std::size_t i = 0; i < num_states; ++i) {
        log_initial[i] = std::log1p(initial[i]);
    }

    // transform transition matrix and emission matrix to log space
    float* log_transition = new float[num_states * num_states];
    for (std::size_t i = 0; i < num_states * num_states; ++i) {
        log_transition[i] = std::log1p(transition[i]);
    }
    float* log_emission = new float[num_states * num_emissions];
    for (std::size_t i = 0; i < num_states * num_emissions; ++i) {
        log_emission[i] = std::log1p(emission[i]);
    }

    // init the likelihood of the observed data
    float log_likelihood = -INFINITY;

    std::size_t iter = 0;
    while (true) {
        auto alpha = forward(observations, num_observations, log_transition, log_emission, log_initial, num_states, num_emissions);
        auto beta = backward(observations, num_observations, log_transition, log_emission, num_states, num_emissions);

        float new_log_likelihood = -INFINITY;
        for (std::size_t i = 0; i < num_states; ++i) {
            new_log_likelihood = log_add(new_log_likelihood, alpha[i * num_observations + (num_observations - 1)]);
        }

        std::cout << "log likelihood: " << log_likelihood << std::endl;
        std::cout << "new log likelihood: " << new_log_likelihood << std::endl;

        // check if the log likelihood is converged
        if (std::abs(std::exp(new_log_likelihood) - std::exp(log_likelihood)) < tolerance) {
            std::cout << "Converged at iteration " << iter << std::endl;

            delete[] alpha;
            delete[] beta;
            break;
        }

        // update the log likelihood
        log_likelihood = new_log_likelihood;

        // check if the max iteration is reached
        if (iter >= max_iters) {
            std::cout << "Max iteration reached." << std::endl;

            delete[] alpha;
            delete[] beta;
            break;
        }

        // compute gamma and xi
        auto gamma = compute_gamma(alpha, beta, num_observations, num_states);
        auto xi = compute_xi(alpha, beta, observations, num_observations, log_transition, log_emission, num_states, num_emissions);

        delete[] alpha;
        delete[] beta;

        // update initial
        for (std::size_t i = 0; i < num_states; ++i) {
            log_initial[i] = gamma[i * num_observations];
        }

        // update transition matrix
        for (std::size_t i = 0; i < num_states; ++i) {
            float denominator = -INFINITY;
            for (std::size_t t = 0; t < num_observations - 1; ++t) {
                denominator = log_add(denominator, gamma[i * num_observations + t]);
            }

            for (std::size_t j = 0; j < num_states; ++j) {
                float numerator = -INFINITY;
                for (std::size_t t = 0; t < num_observations - 1; ++t) {
                    numerator = log_add(numerator, xi[i * num_states * num_observations + j * num_observations + t]);
                }

                log_transition[i * num_states + j] = numerator - denominator;
            }
        }

        // update emission matrix
        for (std::size_t i = 0; i < num_states; ++i) {
            float denominator = -INFINITY;
            for (std::size_t t = 0; t < num_observations; ++t) {
                denominator = log_add(denominator, gamma[i * num_observations + t]);
            }

            for (std::size_t k = 0; k < num_emissions; ++k) {
                float numerator = -INFINITY;
                for (std::size_t t = 0; t < num_observations; ++t) {
                    if (observations[t] == int(k)) {
                        numerator = log_add(numerator, gamma[i * num_observations + t]);
                    }
                }

                log_emission[i * num_emissions + k] = numerator - denominator;
            }
        }

        delete[] gamma;
        delete[] xi;

        ++iter;
    }

    // transform initial from log to normal
    for (std::size_t i = 0; i < num_states; ++i) {
        initial[i] = std::exp(log_initial[i]);
    }

    // transform transition matrix and emission matrix from log to normal
    for (std::size_t i = 0; i < num_states * num_states; ++i) {
        transition[i] = std::exp(log_transition[i]);
    }
    for (std::size_t i = 0; i < num_states * num_emissions; ++i) {
        emission[i] = std::exp(log_emission[i]);
    }

    delete[] log_initial;
    delete[] log_transition;
    delete[] log_emission;
}