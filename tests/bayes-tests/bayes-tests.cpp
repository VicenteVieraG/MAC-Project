#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <bayesClassifier.hpp>
#include <evaluationResult.hpp>

#include <cstddef>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
    std::string fixturePath(const char* name) {
        const auto path = std::filesystem::path(BAYES_TEST_DATA_DIR) / name;
        INFO("Missing fixture: " << path << ". See tests/bayes-tests/README.md.");
        REQUIRE(std::filesystem::is_regular_file(path));
        return path.string();
    }

    void requireSamePerson(const Person& actual, const Person& expected) {
        REQUIRE(actual.age == expected.age);
        REQUIRE(actual.income == expected.income);
        REQUIRE(actual.category == expected.category);
    }
}

TEST_CASE("Bayes training loads the CSV and holds out the final 20 records", "[bayes][training]") {
    const auto path = fixturePath("dataset1.csv");
    BayesClassifier model;
    model.train(path);

    REQUIRE(model.getDataSet().records.size() == 100);
    REQUIRE(model.getRegisterTotal() == 80);
    REQUIRE(model.getTestRecords().size() == 20);
    for (std::size_t i = 0; i < 20; ++i)
        requireSamePerson(model.getTestRecords()[i], model.getDataSet().records[80 + i]);

    SECTION("Retraining discards incremental additions and resets counts") {
        const auto bins = model.getSampleBins();
        model.addPerson({25.0, 10000.0, Category::camina});
        model.train(path);

        REQUIRE(model.getDataSet().records.size() == 100);
        REQUIRE(model.getRegisterTotal() == 80);
        REQUIRE(model.getTestRecords().size() == 20);
        REQUIRE(model.getSampleBins() == bins);
    }
}

TEST_CASE("Bayes partialFit assigns age and income boundaries to the next bin", "[bayes][bins]") {
    // Explicit expected bin indices, independent of the classifier's boundary constants.
    const auto [age, ageBin] = GENERATE(
        std::pair{29.0, 0U}, std::pair{30.0, 1U}, std::pair{31.0, 1U},
        std::pair{59.0, 1U}, std::pair{60.0, 2U}, std::pair{61.0, 2U});
    const auto [income, incomeBin] = GENERATE(
        std::pair{19999.0, 0U}, std::pair{20000.0, 1U}, std::pair{20001.0, 1U},
        std::pair{29999.0, 1U}, std::pair{30000.0, 2U}, std::pair{30001.0, 2U},
        std::pair{39999.0, 2U}, std::pair{40000.0, 3U}, std::pair{40001.0, 3U},
        std::pair{49999.0, 3U}, std::pair{50000.0, 4U}, std::pair{50001.0, 4U});
    const auto category = GENERATE(Category::camina, Category::conduce);
    CAPTURE(age, income, ageBin, incomeBin);

    BayesClassifier model;
    model.partialFit({age, income, category});
    REQUIRE(model.getRegisterTotal() == 1);
    const auto& bins = model.getSampleBins();
    for (std::size_t a = 0; a < bins.size(); ++a)
        for (std::size_t i = 0; i < bins[a].size(); ++i)
            for (std::size_t c = 0; c < bins[a][i].size(); ++c)
                REQUIRE(bins[a][i][c] ==
                    ((a == ageBin && i == incomeBin && c == static_cast<std::size_t>(category)) ? 1 : 0));
}

TEST_CASE("Bayes prediction breaks equal scores in favor of Camina", "[bayes][prediction]") {
    BayesClassifier model;
    model.partialFit({25.0, 10000.0, Category::camina});
    model.partialFit({25.0, 10000.0, Category::conduce});
    const auto label = GENERATE(Category::camina, Category::conduce);
    REQUIRE(model.predict({25.0, 10000.0, label}) == Category::camina);
}

TEST_CASE("Bayes prediction uses priors and smoothing for an unseen bin", "[bayes][prediction]") {
    BayesClassifier model;
    model.partialFit({25.0, 10000.0, Category::camina});
    model.partialFit({65.0, 60000.0, Category::conduce});
    model.partialFit({66.0, 61000.0, Category::conduce});
    // Unseen bin scores: Camina = (1/3)*(1/16), Conduce = (2/3)*(1/17).
    const auto label = GENERATE(Category::camina, Category::conduce);
    REQUIRE(model.predict({45.0, 35000.0, label}) == Category::conduce);
    // Local evidence can still outweigh the larger Conduce prior.
    REQUIRE(model.predict({25.0, 10000.0, label}) == Category::camina);
}

TEST_CASE("Bayes can predict after training on a single class", "[bayes][prediction]") {
    const auto category = GENERATE(Category::camina, Category::conduce);
    BayesClassifier model;
    model.partialFit({25.0, 10000.0, category});
    REQUIRE(model.predict({65.0, 60000.0, Category::camina}) == category);
}

TEST_CASE("Bayes adding a person updates training and preserves the held-out records", "[bayes][training]") {
    BayesClassifier model;
    model.train(fixturePath("dataset1.csv"));
    const auto heldOut = model.getTestRecords();
    auto expectedBins = model.getSampleBins();
    ++expectedBins[0][0][0];
    const Person person{25.0, 10000.0, Category::camina};

    requireSamePerson(model.addPerson(person), person);
    REQUIRE(model.getDataSet().records.size() == 101);
    requireSamePerson(model.getDataSet().records.back(), person);
    REQUIRE(model.getRegisterTotal() == 81);
    REQUIRE(model.getSampleBins() == expectedBins);
    REQUIRE(model.getTestRecords().size() == heldOut.size());
    for (std::size_t i = 0; i < heldOut.size(); ++i)
        requireSamePerson(model.getTestRecords()[i], heldOut[i]);
}

TEST_CASE("Bayes evaluation counts each confusion-matrix outcome", "[bayes][evaluation]") {
    BayesClassifier model;
    model.partialFit({25.0, 10000.0, Category::camina});
    model.partialFit({65.0, 60000.0, Category::conduce});
    const std::vector<Person> records{
        {25.0, 10000.0, Category::camina}, // VP
        {65.0, 60000.0, Category::conduce}, // VN
        {25.0, 10000.0, Category::conduce}, // FP
        {65.0, 60000.0, Category::camina} // FN
    };
    EvaluationResult result;
    REQUIRE_FALSE(result.hasEvaluated());
    // A second evaluation must replace, rather than accumulate, the first.
    for (int run = 0; run < 2; ++run) {
        result.evaluate(model, records);
        REQUIRE(result.hasEvaluated());
        REQUIRE(result.getEvaluatedSampleCount() == 4);
        REQUIRE(result.getVP() == 1);
        REQUIRE(result.getVN() == 1);
        REQUIRE(result.getFP() == 1);
        REQUIRE(result.getFN() == 1);
        REQUIRE(result.getSensibilidad().has_value());
        REQUIRE(result.getEspecificidad().has_value());
        REQUIRE(*result.getSensibilidad() == 0.5);
        REQUIRE(*result.getEspecificidad() == 0.5);
    }

    result.evaluate(model, {});
    REQUIRE(result.hasEvaluated());
    REQUIRE(result.getEvaluatedSampleCount() == 0);
    REQUIRE(result.getVP() == 0);
    REQUIRE(result.getVN() == 0);
    REQUIRE(result.getFP() == 0);
    REQUIRE(result.getFN() == 0);
    REQUIRE_FALSE(result.getSensibilidad().has_value());
    REQUIRE_FALSE(result.getEspecificidad().has_value());
}

TEST_CASE("Bayes evaluation leaves metrics undefined when the actual class is absent", "[bayes][evaluation]") {
    const auto category = GENERATE(Category::camina, Category::conduce);
    BayesClassifier model;
    const Person person{25.0, 10000.0, category};
    model.partialFit(person);
    EvaluationResult result;
    result.evaluate(model, {person});
    if (category == Category::camina) {
        REQUIRE(result.getSensibilidad() == 1.0);
        REQUIRE_FALSE(result.getEspecificidad().has_value());
    } else {
        REQUIRE_FALSE(result.getSensibilidad().has_value());
        REQUIRE(result.getEspecificidad() == 1.0);
    }
}

TEST_CASE("Bayes evaluation rejects an untrained model", "[bayes][evaluation]") {
    BayesClassifier model;
    EvaluationResult result;
    REQUIRE_THROWS_AS(result.evaluate(model, {}), std::logic_error);
    REQUIRE_FALSE(result.hasEvaluated());
}

TEST_CASE("Bayes prediction benchmark", "[bayes][!benchmark][.benchmark]") {
#ifdef DEBUG
    SKIP("Use a Release build for benchmarks; Debug training prints every record.");
#endif
    BayesClassifier model;
    model.train(fixturePath("dataset2.csv"));
    REQUIRE(model.getRegisterTotal() == 800);
    const Person input{45.0, 35000.0, Category::camina};
    BENCHMARK("Predict one person") {
        return model.predict(input);
    };
}

TEST_CASE("Bayes training benchmark", "[bayes][!benchmark][.benchmark]") {
#ifdef DEBUG
    SKIP("Use a Release build for benchmarks; Debug training prints every record.");
#endif
    const auto [name, records] = GENERATE(
        std::pair{"dataset1.csv", 100U}, std::pair{"dataset2.csv", 1000U},
        std::pair{"dataset3.csv", 100000U}, std::pair{"dataset4.csv", 1000000U});
    const auto path = fixturePath(name);
    CAPTURE(name, records);
    BayesClassifier model;
    model.train(path);
    REQUIRE(model.getDataSet().records.size() == records);
    // Includes file I/O, CSV parsing, resetting the model, and fitting.
    // The initial training above warms filesystem caches and allocation capacity.
    BENCHMARK(std::string("Load and train ") + name) {
        model.train(path);
        return model.getRegisterTotal();
    };
}
