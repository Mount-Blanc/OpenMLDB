package com._4paradigm.fesql_auto_test.v1;

import com._4paradigm.fesql.sqlcase.model.SQLCase;
import com._4paradigm.fesql.sqlcase.model.SQLCaseType;
import com._4paradigm.fesql_auto_test.common.FesqlTest;
import com._4paradigm.fesql_auto_test.entity.FesqlDataProviderList;
import com._4paradigm.fesql_auto_test.executor.ExecutorFactory;
import io.qameta.allure.Feature;
import io.qameta.allure.Story;
import lombok.extern.slf4j.Slf4j;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.io.FileNotFoundException;

/**
 * @author zhaowei
 * @date 2020/6/11 2:53 PM
 */
@Slf4j
@Feature("Lastjoin")
public class LastJoinTest extends FesqlTest {

    @DataProvider
    public Object[] testLastJoinData() throws FileNotFoundException {
        FesqlDataProviderList dp = FesqlDataProviderList
                .dataProviderGenerator(new String[]{
                        "/integration/v1/join/",
                       "/integration/cluster/window_and_lastjoin.yaml"
                });
        return dp.getCases().toArray();
    }
    @Story("batch")
    @Test(dataProvider = "testLastJoinData")
    public void testLastJoin(SQLCase testCase) throws Exception {
        ExecutorFactory.build(executor,testCase, SQLCaseType.kBatch).run();
    }
    @Story("request")
    @Test(dataProvider = "testLastJoinData")
    public void testLastJoinRequestMode(SQLCase testCase) throws Exception {
        ExecutorFactory.build(executor,testCase, SQLCaseType.kRequest).run();
    }
    @Story("requestWithSp")
    @Test(dataProvider = "testLastJoinData")
    public void testLastJoinRequestModeWithSp(SQLCase testCase) throws Exception {
        ExecutorFactory.build(executor,testCase, SQLCaseType.kRequestWithSp).run();
    }
    @Story("requestWithSpAysn")
    @Test(dataProvider = "testLastJoinData")
    public void testLastJoinRequestModeWithSpAsync(SQLCase testCase) throws Exception {
        ExecutorFactory.build(executor,testCase, SQLCaseType.kRequestWithSpAsync).run();
    }


}
