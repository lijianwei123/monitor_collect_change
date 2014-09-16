//监控元素采集变化

define(function(require){
	var index = {
		_api_host: '',
		init: function(api_host){
			require('plugins/layer/layer.min');
			this._api_host = api_host;
			this._loadList();
			this.controller();
		},

		_loadList: function() {
			var me = this;
			this.model(api_host + '/collect/list', {}, function(data){
				var obj = {list: data};
				var viewid = me.view('list');
				me.render(viewid, obj, function(content){
					$("#collect_list").html("").html(content);
				});
			});
		},

		controller: function() {
			var me = this;
			//add view
			$('body').on('click', "a.add-monitor", function(){
				var viewid = me.view('add-monitor');
				me.render(viewid, {}, function(content){
					var options = {title: '添加监控'};
					me.display(content, options);
				});
			});

			//insert
			$('body').on("click", '.add-monitor-btn', function(){
				var post_url = me._api_host + '/collect/insert';
				me.model(post_url, $('form#add-monitor-form').serialize(), function(data){
					layer.closeAll();
					L.alert('添加成功');
					window.location.reload();
				}, 'post');
			});

			//修改
			$('body').on('click', '.modify', function(){
				var info = $(this).data('info');
				var viewid = me.view('modify-monitor');
				me.render(viewid, info, function(content){
					var options = {title: '修改监控'};
					me.display(content, options);
				});
			});

			$('body').on('click', '.modify-monitor-btn', function(){
				var post_url = me._api_host + '/collect/modify';
				me.model(post_url, $('form#modify-monitor-form').serialize(), function(data){
					layer.closeAll();
					L.alert('修改成功');
					window.location.reload();
				}, 'post');

			});

			//删除监控
			$('body').on('click', '.del', function(){
				var post_url = me._api_host + '/collect/del';
				var data = {id: $(this).data('id')};
				require.async("plugins/layer/layer.min", function(){
					var index = layer.confirm('亲，你确认删除吗？', function(){
						me.model(post_url, data, function(data){
							layer.closeAll();
							L.alert('删除成功');
							window.location.reload();
						});
						layer.close(index);
					});
				});
			});

			//匹配
			$('body').on('click', '.match', function(){
				var post_url = me._api_host + '/collect/match'
				var data = {id: $(this).data('id')};
				var clickObj = $(this);

				L.loading();
				
				me.model(post_url, data, function(data){
					layer.closeAll();
					if (data.status != undefined) {
						if (data.status == 1) {
							layer.tips('match', clickObj, {guide: 1, time: 2});
						} else {
							layer.tips(data.errMsg, clickObj, {guide: 1, time: 2});
						}
					}
				});
			});
		},

		model: function(url, data, callback){
			L.ajax({
				type: arguments[3] || 'GET',
				url: url,
				cache: true,
				data: data || {},
				success:function(data){
					callback && callback(data);
				}
			});
		},

		view: function(id){
			return id + '-template';
		},

		render : function(viewid, data, callback) {
			L.template(function(template){
				var content = template.render(viewid, data);
				typeof callback == 'function'  && callback(content);
			});
		},

		display : function(content, options){
			options = options || {};
			var _defaultOpt = {
				type : 1,
				title : false,
				border : true,
				shadeClose: true,
				area : ['auto', 'auto'],
				page: {
					html : content
				}
			};
			
			var opt = $.extend({}, _defaultOpt, options);
			
		
			seajs.use("plugins/layer/layer.min", function(layer){
				$.layer(opt);
				if (window.screen.height <= 768) $(".xubox_layer").css("top", 0);
			});
		}
	};

	return index;
});
