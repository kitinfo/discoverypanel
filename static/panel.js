panel = {
	url: "//localhost/kitinfo/discoverypanel/backend/api/",
	init: function() {
		panel.get_trees();
		setInterval(panel.get_trees, 5000);
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
			base: gui.elem("tree_field").value
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
	}
};
