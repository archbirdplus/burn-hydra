#include <gtest/gtest.h>
#include <memory>
#include <iostream>
#include <utility>

#include "fluent.h"
#include "state.h"
#include "kernels.h"

#include "flint/fmpz.h"

class BurnerTest: public testing::Test {
protected:
    CollatzBuilder builder = CollatzBuilder();

    static CollatzBuilder prepareBuilder() {
        return CollatzBuilder()
            .set_flint_threads(1)
            .consistent_collatz(3, 2, {0, 1})
            .set_iterations(256)
            .set_table_size(2)
            .do_prune(true)
            .set_initial(3);
    }

    void SetUp() override {
        builder = prepareBuilder();
    }

    void TearDown() override {
        
    }
public:
    BurnerTest() {
        SetUp();
    }
};

void expect_timed_eq(timed_fmpz& lhs, timed_fmpz& rhs) {
    EXPECT_EQ(lhs.iterations, rhs.iterations) << "integers are not synced";
    char* strL = fmpz_get_str(NULL, 10, &lhs.value);
    char* strR = fmpz_get_str(NULL, 10, &rhs.value);
    EXPECT_TRUE(fmpz_equal(&lhs.value, &rhs.value)) << strL << " is not equal to " << strR;
    free(strL);
    free(strR);
}

void expect_timed_eq_val_time(timed_fmpz& lhs, uint64_t rhs, uint64_t time) {
    EXPECT_EQ(lhs.iterations, time) << "integers are not synced";
    char* str = fmpz_get_str(NULL, 10, &lhs.value);
    EXPECT_TRUE(fmpz_equal_ui(&lhs.value, rhs)) << str << " is not equal to " << str;
    free(str);
}

void expect_fmpz_eq_ui(fmpz* lhs, uint64_t rhs) {
    char* str = nullptr;
    EXPECT_TRUE(fmpz_equal_ui(lhs, rhs)) << (str = fmpz_get_str(NULL, 10, lhs)) << " is not equal to " << rhs;
}

typedef struct count_context {
    uint64_t even;
    uint64_t odd;
} count_context_t;

user_object_t count_parities(void* context, user_object_t x, uint64_t residue) {
    const auto ctx = (count_context_t*) context;
    if (residue & 1) {
        ctx->odd += 1;
    } else {
        ctx->even += 1;
    }
    return x;
}

user_object_t count_parities_fancy(void* context, user_object_t x, uint64_t residue) {
    const auto ctx = (count_context_t*) context;
    if (residue & 1) {
        ctx->odd += 1;
    } else {
        ctx->even += 1;
    }
    if ((3*residue/2) & 1) {
        ctx->odd += 1;
    } else {
        ctx->even += 1;
    }
    return x;
}

TEST_F(BurnerTest, CountParities) {
    count_context_t counts = { 0, 0 };
    Context context = builder
        .block_sizes({{8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19}}, {})
        .set_iterations((1<<20))
        .set_table_size(1)
        .scan_fn(&count_parities, 1, false)
        .scan_context(&counts)
        .init();
    context.run();

    // Including initial step but not H^(2^20)(3)
    EXPECT_EQ(counts.even, 523508);
    EXPECT_EQ(counts.odd, 525068);
}

TEST_F(BurnerTest, CountParitiesFancy) {
    count_context_t counts = { 0, 0 };
    Context context = builder
        .block_sizes({{8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19}}, {})
        .set_iterations((1<<20))
        .set_table_size(2)
        .scan_fn(&count_parities_fancy, 2, false)
        .scan_context(&counts)
        .init();
    context.run();

    // Including initial step but not H^(2^20)(3)
    EXPECT_EQ(counts.even, 523508);
    EXPECT_EQ(counts.odd, 525068);
}

template <typename T>
class BurnerTypesTest: public BurnerTest {

};

TYPED_TEST_SUITE_P(BurnerTypesTest);

using BurnerTypes = ::testing::Types<
    std::tuple<Burner_singlethreaded, Basecase_simple>,
    std::tuple<Burner_singlethreaded, Basecase_m2exp>,
    std::tuple<Burner_singlethreaded, Basecase_table>,
    std::tuple<Burner_m2exp, Basecase_simple>,
    std::tuple<Burner_m2exp, Basecase_m2exp>,
    std::tuple<Burner_m2exp, Basecase_table>
>;
// Unfortunately, it seems we can't have such a loop:
// ::testing::Combine(::testing::Types<Burner_singlethreaded, Burner_m2exp>, ::testing::Types<Basecase_simple, Basecase_m2exp, Basecase_table>);

TYPED_TEST_P(BurnerTypesTest, CheckFinalValue) {
    uint64_t iteration_count = 192;
    auto builder = CollatzBuilder()
            .set_flint_threads(1)
            .consistent_collatz(3, 2, {0, 1})
            .set_iterations(iteration_count)
            .set_table_size(2)
            .do_prune(true)
            .set_initial(3);
    Context context = builder.block_sizes({{4, 5, 6}}, {}).init();
    // burner:   simple, m2exp
    // basecase: simple, table, m2exp
    // threaded should be an on-top layer
    using BurnerType = typename std::tuple_element<0, TypeParam>::type;
    using BaseType = typename std::tuple_element<1, TypeParam>::type;
    auto burner = BurnerType(
        &context,
        std::unique_ptr<Burner_MPI>(new Burner_MPI(&context)),
        std::unique_ptr<Basecase_simple>(new BaseType(&context))
    );
    uint64_t iterations = 0;
    uint64_t steps = 0;
    while(iterations < iteration_count) {
        std::cout << "step " << steps << std::endl;
        iterations += burner.step();
        steps += 1;
    }
    EXPECT_EQ(iterations, iteration_count);

    fmpz_t tmp; fmpz_init(tmp);
    timed_fmpz result = timed_fmpz();
    // Currently overcarries are only computed for pushL (which does
    // not necessarily happen on the last iteration) but undecarries
    // are always extracted.
    fmpz_one_2exp(tmp, 0);
    fmpz_addmul(&result.value, &burner.basecase_context->storage.value, tmp);
    fmpz_addmul(&result.value, &burner.undercarry[0].value, tmp);
    fmpz_one_2exp(tmp, (1<<4)*1);
    fmpz_addmul(&result.value, &burner.storage[0].value, tmp);
    fmpz_addmul(&result.value, &burner.undercarry[1].value, tmp);
    fmpz_one_2exp(tmp, (1<<4)*2);
    fmpz_addmul(&result.value, &burner.storage[1].value, tmp);
    fmpz_addmul(&result.value, &burner.undercarry[2].value, tmp);
    fmpz_one_2exp(tmp, (1<<4)*2+(1<<5));
    fmpz_addmul(&result.value, &burner.storage[2].value, tmp);

    timed_fmpz answer = timed_fmpz();
    fmpz_set_uiui(&answer.value, 850778579484107, 1983176903683680569);
    expect_timed_eq(answer, result);
}

REGISTER_TYPED_TEST_SUITE_P(BurnerTypesTest, CheckFinalValue);

INSTANTIATE_TYPED_TEST_SUITE_P(Standard, BurnerTypesTest, BurnerTypes);


