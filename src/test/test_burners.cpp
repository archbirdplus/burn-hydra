#include <gtest/gtest.h>
#include <memory>
#include <iostream>

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
            .consistent_collatz(3, 2, {0, 0})
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

void expect_fmpz_eq_ui(fmpz* lhs, uint64_t rhs) {
    char* str = nullptr;
    EXPECT_TRUE(fmpz_equal_ui(lhs, rhs)) << (str = fmpz_get_str(NULL, 10, lhs)) << " is not equal to " << rhs;
}

TEST_F(BurnerTest, OneBlockRun) {
    Context context = builder.block_sizes({{4}}, {}).init();
    Burner_MPI burner = Burner_MPI(&context);
    expect_fmpz_eq_ui(burner.basecase_context->storage, 3);
    expect_fmpz_eq_ui(&burner.node_context->storage[0], 0);
    EXPECT_EQ(burner.node_context->storage.size(), 1);
    EXPECT_EQ(burner.step(), 16);
    EXPECT_EQ(burner.basecase_context->power, 4);
    // mod 2^2^power = 2^16 = 65536
    expect_fmpz_eq_ui(burner.basecase_context->storage, 1599);
    expect_fmpz_eq_ui(&burner.node_context->storage[0], 0);
    EXPECT_EQ(burner.step(), 16);
    expect_fmpz_eq_ui(burner.basecase_context->storage, 1293);
    expect_fmpz_eq_ui(&burner.node_context->storage[0], 16);
    EXPECT_EQ(burner.step(), 16);
    expect_fmpz_eq_ui(burner.basecase_context->storage, 26576);
    expect_fmpz_eq_ui(&burner.node_context->storage[0], 10522);
}

/*
TEST_F(BurnerTest, TwoBlockRun) {
    Context context = builder.block_sizes({{4, 4}}, {}).init();
    Burner_MPI burner = Burner_MPI(&context);
    expect_fmpz_eq_ui(burner.basecase_context->storage, 3);
    expect_fmpz_eq_ui(&burner.node_context->storage[0], 0);
    expect_fmpz_eq_ui(&burner.node_context->storage[1], 0);
    EXPECT_EQ(burner.step(), 16);
    EXPECT_EQ(burner.basecase_context->power, 4);
    expect_fmpz_eq_ui(burner.basecase_context->storage, 1599);
    expect_fmpz_eq_ui(&burner.node_context->storage[0], 0);
    expect_fmpz_eq_ui(&burner.node_context->storage[1], 0);
    EXPECT_EQ(burner.step(), 16);
    expect_fmpz_eq_ui(burner.basecase_context->storage, 1293);
    expect_fmpz_eq_ui(&burner.node_context->storage[0], 16);
    expect_fmpz_eq_ui(&burner.node_context->storage[1], 0);
    EXPECT_EQ(burner.step(), 16);
    expect_fmpz_eq_ui(burner.basecase_context->storage, 26576);
    expect_fmpz_eq_ui(&burner.node_context->storage[0], 10522);
    expect_fmpz_eq_ui(&burner.node_context->storage[1], 0);
    EXPECT_EQ(burner.step(), 16);
    expect_fmpz_eq_ui(burner.basecase_context->storage, 41358);
    expect_fmpz_eq_ui(&burner.node_context->storage[0], 30265);
    expect_fmpz_eq_ui(&burner.node_context->storage[1], 105);
    EXPECT_EQ(burner.step(), 16);
    expect_fmpz_eq_ui(burner.basecase_context->storage, 26096);
    expect_fmpz_eq_ui(&burner.node_context->storage[0], 41151); // first point of failure
    expect_fmpz_eq_ui(&burner.node_context->storage[1], 69271);
    EXPECT_EQ(burner.step(), 16);
    expect_fmpz_eq_ui(burner.basecase_context->storage, 50647);
    expect_fmpz_eq_ui(&burner.node_context->storage[0], 63221);
    expect_fmpz_eq_ui(&burner.node_context->storage[1], 45500433);
}
*/

TEST_F(BurnerTest, IndividualSyncs) {
    Context context = builder.block_sizes({{4, 5}}, {}).init();
    Burner_MPI burner = Burner_MPI(&context);
    fmpz_set_ui(burner.basecase_context->storage, 3);
    fmpz_set_ui(&burner.node_context->storage[0], 7);
    fmpz_set_ui(&burner.node_context->storage[1], 13);

    burner.node_context->syncR(0);
    expect_fmpz_eq_ui(&burner.node_context->undercarry[0], 58055);
    expect_fmpz_eq_ui(&burner.node_context->overcarry[0], 0);
    expect_fmpz_eq_ui(&burner.node_context->overcarry[1], 0);
    expect_fmpz_eq_ui(&burner.node_context->undercarry[1], 0);
    expect_fmpz_eq_ui(burner.basecase_context->storage, 59654); // 1599+58055
    expect_fmpz_eq_ui(&burner.node_context->storage[0], 4597);
    expect_fmpz_eq_ui(&burner.node_context->storage[1], 13);
    burner.node_context->syncL(0); // does nothing yet
    expect_fmpz_eq_ui(&burner.node_context->undercarry[0], 58055);
    expect_fmpz_eq_ui(&burner.node_context->overcarry[0], 0);
    expect_fmpz_eq_ui(&burner.node_context->overcarry[1], 0);
    expect_fmpz_eq_ui(&burner.node_context->undercarry[1], 0);
    expect_fmpz_eq_ui(burner.basecase_context->storage, 59654);
    expect_fmpz_eq_ui(&burner.node_context->storage[0], 4597);
    expect_fmpz_eq_ui(&burner.node_context->storage[1], 13);
    burner.node_context->syncR(1);
    expect_fmpz_eq_ui(&burner.node_context->undercarry[0], 58055);
    expect_fmpz_eq_ui(&burner.node_context->overcarry[0], 0);
    expect_fmpz_eq_ui(&burner.node_context->overcarry[1], 0);
    expect_fmpz_eq_ui(&burner.node_context->undercarry[1], 61005);
    expect_fmpz_eq_ui(burner.basecase_context->storage, 59654);
    expect_fmpz_eq_ui(&burner.node_context->storage[0], 4597);
    expect_fmpz_eq_ui(&burner.node_context->storage[1], 8538);
    burner.node_context->syncR(0);
    expect_fmpz_eq_ui(&burner.node_context->undercarry[0], 21045);
    expect_fmpz_eq_ui(&burner.node_context->overcarry[0], 598);
    expect_fmpz_eq_ui(&burner.node_context->overcarry[1], 0);
    expect_fmpz_eq_ui(&burner.node_context->undercarry[1], 61005);
    expect_fmpz_eq_ui(burner.basecase_context->storage, 13358);
    // TODO: below should have +61005 first, s.t. the result is 43090072
    expect_fmpz_eq_ui(&burner.node_context->storage[0], 3020095);
    expect_fmpz_eq_ui(&burner.node_context->storage[1], 8538);
    burner.node_context->syncL(0);
    burner.node_context->syncR(1);
    burner.node_context->syncL(1);

    expect_fmpz_eq_ui(burner.basecase_context->storage, 13358); //, 24763);
    expect_fmpz_eq_ui(&burner.node_context->storage[0], 66444); // 37064);
    expect_fmpz_eq_ui(&burner.node_context->storage[1], 5608153); // 5608764);
}

/*
TEST_F(BurnerTest, FullStep) {
    Context context = builder.block_sizes({{4, 5}}, {}).init();
    Burner_MPI burner = Burner_MPI(&context);
    fmpz_set_ui(burner.basecase_context->storage, 3);
    fmpz_set_ui(&burner.node_context->storage[0], 7);
    fmpz_set_ui(&burner.node_context->storage[1], 13);
    EXPECT_EQ(burner.step(), 32);
    expect_fmpz_eq_ui(burner.basecase_context->storage, 24763);
    expect_fmpz_eq_ui(&burner.node_context->storage[0], 37064);
    expect_fmpz_eq_ui(&burner.node_context->storage[1], 5608764);
}
*/

/*
TEST_F(BurnerTest, UnevenBlockRun) {
    Context context = builder.block_sizes({{4, 5}}, {}).init();
    Burner_MPI burner = Burner_MPI(&context);
    expect_fmpz_eq_ui(burner.basecase_context->storage, 3);
    expect_fmpz_eq_ui(&burner.node_context->storage[0], 0);
    expect_fmpz_eq_ui(&burner.node_context->storage[1], 0);
    EXPECT_EQ(burner.step(), 32);
    expect_fmpz_eq_ui(burner.basecase_context->storage, 1293);
    expect_fmpz_eq_ui(&burner.node_context->storage[0], 16);
    expect_fmpz_eq_ui(&burner.node_context->storage[1], 0);
    EXPECT_EQ(burner.step(), 32);
    expect_fmpz_eq_ui(burner.basecase_context->storage, 41358);
    expect_fmpz_eq_ui(&burner.node_context->storage[0], 30265);
    expect_fmpz_eq_ui(&burner.node_context->storage[1], 105);
}
*/

// TODO: check parities are exact

