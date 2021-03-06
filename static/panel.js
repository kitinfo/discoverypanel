panel = {
	url: "//localhost/kitinfo/discoverypanel/backend/api/",
	init: function() {
		if (!window.location.hash || window.location.hash === '#') {
			window.location.hash = '#files';
		}
		panel.get_trees();
		panel.get_files();
		setInterval(panel.get_trees, 5000);
		setInterval(panel.get_files, 5000);
	},
	get_files: function() {
		panel.fill_tags();
		var file_filter = gui.elem('file_filter').value;
		var tags_filter = gui.elem('tags_filter').value;
		var status_filter = gui.elem('status_filter').value;
		ajax.asyncGet(`${panel.url}?files&file=${file_filter}&tag=${tags_filter}&status=${status_filter}`, function(xhr) {
			var files = JSON.parse(xhr.response).files;

			var tbody = gui.elem("files_body");
			tbody.innerHTML = '';
			files.forEach(function(file) {
				var row = gui.create('tr');
				var linkElem = gui.create('td');
				var link = file.base;
				if (file.base[file.base.length -1] !== '/') {
					link += '/';
				}
				link += file.path;
				linkElem.appendChild(gui.createLink(decodeURI(link), link));
				row.appendChild(linkElem);
				var tags = gui.create('td');
				tags.textContent = file.tags;
				row.appendChild(tags);
				var status = gui.create('td');
				status.textContent = file.status;
				row.appendChild(status);

				tbody.appendChild(row);
			});
		});
	},
	get_trees: function() {
		ajax.asyncGet(panel.url + "?trees", function(xhr) {
			var trees = JSON.parse(xhr.response).trees;

			var tbody = gui.elem("trees_body");
			tbody.innerHTML = "";
			trees.forEach(function(tree) {
				var row = gui.create("tr");

				var basetd = gui.create("td");
				basetd.appendChild(gui.createLink(tree.base, tree.base));
				row.appendChild(basetd);
				row.appendChild(gui.createColumn(tree.comment));
				row.appendChild(gui.createColumn(tree.status));

				tbody.appendChild(row);
			});
		});
	},
	hide_add_tree: function() {
		gui.elem("add_tree").style.display = "none";
	},
	add_tree: function() {
		var obj = {
			base: gui.elem("tree_field").value,
			comment: gui.elem("tree_comment").value
		};

		if (obj.base == "") {
			gui.elem("status_box").textContent = "Please enter a tree.";
			return;
		}

		ajax.asyncPost(panel.url + "?add_tree", JSON.stringify(obj), function(xhr) {
			var status = JSON.parse(xhr.response).status;
			gui.elem("status_box").textContent = status;

			if (status == "ok") {
				panel.hide_add_tree();
				panel.get_trees();
			}
		});
	},
	show_add_tree: function() {
		gui.elem("tree_field").value = "";
		gui.elem("add_tree").style.display = "block";
	},
	fill_datalist: function(elem, values, key) {
		elem.innerHTML = '';
		values.forEach(function(value) {
			elem.appendChild(gui.createOption('', value[key]));
		});
	},
	fill_tags: function() {
		var self = this;
		var elem = gui.elem('tags_datalist');
		ajax.asyncGet(`${panel.url}?tags`, function(xhr) {
				var tags = JSON.parse(xhr.response).tags;

				self.fill_datalist(elem, tags, 'tag_text');
		});
	}
};
