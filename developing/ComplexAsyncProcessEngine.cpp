
//-- Reference:
//-- 	https://developer.mozilla.org/zh-CN/docs/Web/JavaScript/Guide/Using_promises
//--	https://developer.mozilla.org/zh-CN/docs/Web/JavaScript/Reference/Global_Objects/Promise

///////////----- Type: I -----/////////////////////////
class ComplexAsyncProcessI
{
	std::unordered_map<string, std::function> _funcMap;

public:
	ComplexAsyncProcessI& addProcess(const st::string& name, std::function&& func);
	ComplexAsyncProcessI& call(const std::string& name, arg&&...);
	ComplexAsyncProcessI& done();
	ComplexAsyncProcessI& fail();
};



///////////----- Type: II -----/////////////////////////
class ComplexAsyncProcessII
{
public:
	ComplexAsyncProcessII& then(std::function&& func, std::function&& failureCallback = nullptr);
	ComplexAsyncProcessII& then(std::list<std::function>& funcs, std::function&& failureCallback = nullptr);
	ComplexAsyncProcessII& done();
	ComplexAsyncProcessII& fail();

	void parallel(func1, func2, ...);
};

addProcess: 添加命名函数
create：创建对象
launch/start：开始执行
call：调用函数、并发调用函数
done：等待前面完成
catch：前面错误处理
finally：资源释放收尾
async：异步触发流程，不等待完成
wait：等待特定流程完成

结合client.sendQuest()的回调处理。

TypeIII::create().call(func).call(func1, func2, ..., funcN).done().finally().launch();
TypeIII::create().call(func).catch(func).call(func1, func2, ..., funcN).done().finally().launch();

auto ac = TypeIV::create().call(func).catch(func).async(func1, func2, func3).call(func);
ac.wait(func1).call(func).async(func4, func5, func6).call(func);
ac.call(func).wait(func2, func4, fcun5).call(func);
ac.wait(func3, func6).done().finally().launch();


TypeIII::create().call(funcName).call(funcName1, funcName2, ..., ffuncNameN).done().finally().launch();
TypeIII::create().call(funcName).catch(funcName).call(funcName1, funcName2, ..., funcNameN).done().finally().launch();

auto ac = TypeIV::create();
ac.addProcess(functName, func);
ac.addProcess(functName1, func1);
... ... ...
ac.call(funcName).catch(funcName).async(funcName1, funcName2, funcName3).call(funcName);
ac.wait(funcName1).call(funcName).async(funcName4, funcName5, funcName6).call(funcName);
ac.call(funcName).wait(funcName2, funcName4, funcName5).call(funcName);
ac.wait(funcName3, funcName6).done().finally().launch();